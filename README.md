# local-contest-archive

Local Contest Archive (LCA) is a command-line tool for downloading, organizing and searching competitive programming contests from multiple sources.

## Planned features

- Download contests from multiple providers
  - Codeforces
  - AtCoder
  - QOJ
- Local archive
- Search by contest, problem and topic
- Bookmarks
- Notes
- Workspace
- Continue unfinished problems

## Example

```bash
lca archive update

lca archive list

lca archive install ioi

lca search centroid

lca work CF_2000_A
```

## Requirements

- C++23
- CMake
- libcurl
- Node.js
- Playwright

Install Playwright browsers with:

```bash
cd tools
npm install
npx playwright install chromium
```

## Converting statements

Statements are stored as raw HTML (or PDF for some problems) after download.

Convert them to Markdown with:

```bash
lca convert          # all contests
lca convert 1791     # one contest
```

Each statement.html creates a statement.md beside it, with math kept as readable text and images downloaded into a local statement_files/ directory. Problems whose statement is PDF-only are skipped and reported.

## Notes

Codeforces statements are downloaded using Playwright because direct HTTP requests are protected by Cloudflare.

## Building

```bash
mkdir build
cd build
cmake ..
make
```
