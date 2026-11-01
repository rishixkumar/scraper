# Recall

A Spotlight-style search engine for lecture slides, PDFs, notes, and code on your own machine. A background daemon keeps a C++ inverted index live as files change, and a CLI named `recall` answers queries in milliseconds.

```text
recall "page tables" --course 2110
```

That returns ranked hits with snippets and file paths. The point of the tool is the 2am case: finding the one slide about page tables before an exam, without remembering which folder it lives in.

Repository: [github.com/rishixkumar/scraper](https://github.com/rishixkumar/scraper)

## Status

Checkpoint 0 is the repository itself: this plan, a C++/CMake ignore list, and a `main` branch. No indexer code yet.

| Phase | Goal | Done when |
| --- | --- | --- |
| 0 | Repository, plan, layout | This README is on `main` |
| 1 | Crawler, text extractors, tokenizer | Tokens dump from a folder of mixed files |
| 2 | In-memory inverted index, BM25, CLI query | Search works, with no persistence yet |
| 3 | On-disk segments, mmap, compression, crash safety | Survives restart and `kill -9` |
| 4 | Live updates and startup reconciliation | An edit is searchable within about 1 second |
| 5 | Daemon, socket IPC, thread pool, snapshots | Background process stays up under load |
| 6 | Typo tolerance, phrases, snippets, benchmarks | It feels like a product, with measured numbers |

Phases 1–4 are the resume project. Phases 5–6 are the polish. Part-time, expect about 6–10 weeks if the later phases go deep.

## What it is

A daemon indexes the files you care about (slides, PDFs, notes, code, markdown), updates that index as files change, and serves queries from a CLI. It is a local full-text engine, not a web scraper and not a cloud search product.

Daily loop:

1. Point it at course folders, notes, and code trees.
2. Leave it running at low priority.
3. Query from the terminal when you need a specific slide, note, or function.

## Architecture

```text
          ┌──────────────┐
          │  watcher     │  create / modify / move / delete
          └──────┬───────┘
                 │ debounced events
          ┌──────▼───────┐        ┌────────────────────┐
          │ Work queue   │───────▶│ Extractor workers  │  PDF / PPTX / DOCX / text / code
          └──────────────┘        │ (thread pool)      │  → plain text → tokens
                                  └─────────┬──────────┘
                                            │ token batches
                                  ┌─────────▼──────────┐
                                  │ Index writer       │  in-memory segment, then
                                  │ (single thread)    │  immutable on-disk segments
                                  └─────────┬──────────┘
                                            │ atomic snapshot swap
          ┌──────────────┐        ┌─────────▼──────────┐
          │ CLI (`recall`)│◀─────▶│ Query server       │  Unix domain socket
          └──────────────┘        │ (reader threads)   │  mmap'd segments
                                  └────────────────────┘
```

One writer, many readers. Readers hold an immutable snapshot (an atomic `shared_ptr` swap, RCU-style). A query never waits on indexing, and an in-flight query never sees a half-written segment. That snapshot rule is the concurrency story to be able to explain out loud.

## Component analysis

### 1. File watcher

`inotify` is not recursive. The watcher adds one watch per directory and adds new watches when directories appear. The events that matter are `IN_CLOSE_WRITE`, `IN_MOVED_FROM` / `IN_MOVED_TO`, `IN_DELETE`, and especially `IN_Q_OVERFLOW`. Overflow means the kernel dropped events, so the only correct response is a full rescan of the affected tree.

Editors save through a temp file plus rename, so one save can emit several events. Debounce by path before enqueueing work. Large trees hit `fs.inotify.max_user_watches`; detect the limit and degrade (narrower roots, or a rescan fallback) instead of failing silently.

On startup the daemon was off while files changed. Reconcile by comparing mtime, size, and inode with the indexed record.

Stretch goal: a `fanotify` backend. It sees more of the filesystem, and it needs elevated privileges. Write the tradeoff down when it exists; do not block earlier phases on it.

**This machine is macOS.** `inotify`, `fanotify`, `systemd`, `ionice`, and cgroups are Linux facilities. The indexer, tokenizer, segments, and query engine stay portable C++20 so they build and test here. The watcher sits behind an interface:

- Linux: `inotify`, with `fanotify` as a later backend
- macOS: FSEvents for real use on this laptop, plus a polling fallback if a watch cannot be installed

A systemd user unit is the Linux service. The macOS equivalent is a launchd agent. Resume language that cites `inotify` has to be measured on Linux (a VM or container is enough).

### 2. Text extraction

Plain text, markdown, and source code are read directly. PPTX and DOCX are zip archives of XML (`libzip` + pugixml). PDFs go through poppler (`libpoppler-cpp`).

Skip binaries, files over a size cap, `.git`, `node_modules`, and anything on a configurable ignore list. Skip scanned PDFs. OCR is out of scope. PDF extraction is the slowest stage, so it is the first work to put on the thread pool.

### 3. Tokenizer

Lowercase, split on punctuation, and split code identifiers. `pageTableEntry` and `page_table_entry` both yield `page`, `table`, `entry`. Stemming is optional and off until ranking quality needs it. The same tokenizer must run at index time and query time.

### 4. Index

Segment-based, in the same family as Lucene and LSM trees.

- New documents land in an in-memory segment.
- That segment flushes to an immutable on-disk segment.
- Deletes and replacements are tombstones.
- A background merge compacts segments.

Each segment stores a sorted term dictionary and postings: document id, term frequency, and positions. Postings are delta-encoded and varint-compressed. Segments are mmap'd so the page cache is the cache, with `madvise` hints for how queries actually touch the file.

Crash safety is a hard requirement. Write the segment to a temp file, `fsync`, atomically rename it, then update a manifest the same way. `kill -9` mid-write must leave the previous manifest pointing at a complete segment. That gets a direct test, not a comment.

### 5. Query and ranking

- BM25 over term frequency, document length, and corpus statistics stored with the snapshot
- Boolean queries
- Phrase queries, which are why positions are stored
- Snippets with the match highlighted
- Prefix search for as-you-type

Typo tolerance, in the order to build it:

1. Trigram index for candidates, then confirm with Damerau-Levenshtein
2. SymSpell-style delete neighborhoods
3. A Levenshtein automaton walked over a trie or FST of the term dictionary

Ship (1) before attempting (3).

### 6. Concurrency

Single index writer. Readers copy a `shared_ptr` to the current snapshot and never lock against the writer. The writer publishes a new snapshot only after the new segment is durable and the manifest is updated. Explain this in later READMEs with the actual types, because it is the interview talking point: snapshot isolation and RCU-style reads.

### 7. Daemon and CLI

The daemon listens on a Unix domain socket with a length-prefixed frame protocol. It runs at low priority (`nice` / `ionice` on Linux, a low QoS launchd job on macOS) so a reindex does not stall compiles or notes.

CLI:

- `recall <query>`
- `recall status`
- `recall reindex`
- `recall exclude <path>`

An exclude list exists from day one. This process reads file contents. SSH keys, credential files, password stores, and `.env` files are excluded by default.

## Planned layout

```text
recall/
  CMakeLists.txt
  src/
    crawl/        walk roots, apply ignore rules, stat identity
    extract/      text, code, pdf, pptx, docx
    tokenize/     shared analyzer
    index/        segments, postings, varint, manifest
    query/        BM25, phrases, snippets, typo
    watch/        watcher interface, inotify, fsevents
    daemon/       queue, writer thread, socket server
    cli/          recall client
  tests/          GoogleTest, fuzz targets, crash-recovery
  bench/          throughput, latency, compare vs ripgrep and FTS5
```

The folder on disk today is `job scraper`. The binary and the library are `recall`. The GitHub repo name stays `scraper`.

## Tech stack

C++20, CMake, GoogleTest, libpoppler-cpp, libzip, pugixml. Linux adds a systemd user unit. Sanitizers: ASan, UBSan, and ThreadSanitizer on the concurrency tests. Fuzz the varint decoder and the tokenizer.

Dependencies stay out of the first commit. Phase 1 adds the ones the extractors need, found with `pkg-config` where the platform provides them.

## Benchmarks

Do not fill resume numbers until they are measured.

Corpus: personal course files, plus the Linux kernel tree for code and a Wikipedia subset for prose.

Measure:

- Indexing throughput (MB/s)
- Query latency (p50 / p99)
- Resident memory
- Index size versus corpus size
- Delay until a live edit is searchable

Compare with ripgrep and SQLite FTS5. ripgrep will win some of these. Say so. Credibility is "ripgrep wins at X, Recall wins at Y" (persistent index, typo tolerance, snippets, live updates).

Resume shape, brackets left empty on purpose:

> Recall: Local Full-Text Search Daemon | C++20, Linux
>
> Built a background search engine indexing [X] files / [Y] GB with a segment-based inverted index (varint-compressed postings, mmap'd, BM25 ranking), returning typo-tolerant results in [Z] ms p99.
> Implemented live re-indexing via inotify with event debouncing and crash-safe atomic segment commits; verified with kill -9 recovery tests.
> Designed lock-free reader snapshots so queries never block on indexing; benchmarked against ripgrep and SQLite FTS5.

## Testing bar

- GoogleTest for units (tokenizer, varint, BM25, manifest)
- Fuzz varint decode and tokenization
- ASan / UBSan on the library; ThreadSanitizer on snapshot publish
- A crash-recovery test that kills the writer mid-`fsync` / mid-rename and checks the manifest still opens

## Gotchas already decided

- Debounce editor save storms before they hit the extractor pool.
- Cap file size, refuse symlink loops, and bound directory depth.
- Parallelize PDF extraction first.
- Default deny for secrets: keys, cookies, `.env`, password databases.
- `IN_Q_OVERFLOW` and a missed macOS event stream both mean "rescan", never "assume the index is complete".

## How this repo will grow

Each phase ends with a README update (status table, how to build, how to run what exists) and a push to `main` on `rishixkumar/scraper`. Phase 1 is next: a crawler, extractors, and tokenizer that can print tokens for a mixed folder, with tests, still with no index.
