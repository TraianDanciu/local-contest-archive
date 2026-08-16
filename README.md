# local-contest-archive

Local Contest Archive (LCA) is a command-line tool for downloading, organizing and searching competitive programming contests from multiple sources.

## Planned features

- Download contests from multiple providers
  - Codeforces
  - AtCoder
  - Online OI database
- Local archive
- Search by contest, problem and topic
- Bookmarks
- Notes
- Workspace
- Continue unfinished problems

## Example

```bash
lca oi list ioi        # years available in the online database

lca oi update ioi      # download all IOI years

lca oi update ejoi 2020 2021   # only those years

lca atcoder update abc001 abc002  # several contests at once

lca codeforces convert 1791 2000  # convert several contests at once

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

## Olympiad archive (online database)

`lca oi` downloads contests from the LCA online database into
`archive/<olympiad>/<year>/`. Statements are PDFs and are kept as-is.

```bash
lca oi list                        # all olympiads
lca oi list ioi                    # IOI years and total size
lca oi update ioi                  # download all IOI years
lca oi update ejoi 2020 2021       # download selected years
lca oi status                      # show which olympiad years are complete
```

Supported olympiads:

| id | Olympiad |
|----|----------|
| `apio` | Asia-Pacific Informatics Olympiad |
| `baltoi` | Baltic Olympiad in Informatics |
| `ceoi` | Central European Olympiad in Informatics |
| `egoi` | European Girls' Olympiad in Informatics |
| `ejoi` | European Junior Olympiad in Informatics |
| `ioi` | International Olympiad in Informatics |
| `joi` | JOI Final (English) |
| `joioc` | JOI Open Contest |
| `joisc` | JOI Spring Camp (English) |

Each olympiad year is stored as a single archive in the database. `lca oi update`
downloads the years that are missing locally into `archive/<olympiad>/<year>/`,
so a rerun only fetches what is missing.

## Converting statements

Statements are stored as raw HTML (or PDF for some problems) after download.

Convert them to Markdown with:

```bash
lca convert                          # all contests
lca codeforces convert 1791 2000     # several contests
lca atcoder convert abc001 abc002
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
