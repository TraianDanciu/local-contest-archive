const { chromium } = require('playwright');
const fs = require('fs');

const CONCURRENCY = 5;
const NAV_TIMEOUT = 90000;
const BATCH_SIZE = 250;

async function main() {
  if(process.argv.length != 3) {
    console.error("Usage: node fetch_statement.js <jobfile>");
    process.exit(1);
  }

  const jobs = [];
  for(const line of fs.readFileSync(process.argv[2], 'utf8').split('\n')) {
    const trimmed = line.trim();
    if(!trimmed) {
      continue;
    }

    const tab = trimmed.indexOf('\t');
    if(tab < 0) {
      console.error("Skipping bad line (no tab): " + trimmed);
      continue;
    }

    jobs.push({
      url: trimmed.slice(0, tab),
      output: trimmed.slice(tab + 1)
    });
  }

  console.error("Fetching " + jobs.length + " statements...");

  const failed = [];
  let done = 0;
  const start = Date.now();

  async function fetchOne(browser, job) {
    let page = null;
    try {
      page = await browser.newPage();

      const pdfs = [];
      page.on('response', async (resp) => {
        const ct = resp.headers()['content-type'] || '';
        if(ct.includes('pdf')) {
          try {
            const buf = await resp.body();
            if(buf.slice(0, 5).toString() === '%PDF-') {
              pdfs.push(buf);
            }
          } catch(e) {}
        }
      });

      await page.goto(job.url, { waitUntil: 'domcontentloaded', timeout: NAV_TIMEOUT });

      const deadline = Date.now() + NAV_TIMEOUT;
      let saved = false;
      while(Date.now() < deadline) {
        if(pdfs.length > 0) {
          fs.mkdirSync(require('path').dirname(job.output), { recursive: true });
          fs.writeFileSync(job.output.replace(/\.html$/, '') + '.pdf', pdfs[0]);
          saved = true;
          break;
        }

        const count = await page.locator('.problem-statement').count();
        if(count > 0) {
          const statement = await page.locator('.problem-statement').innerHTML();
          if(statement && !statement.includes('Just a moment')) {
            fs.mkdirSync(require('path').dirname(job.output), { recursive: true });
            fs.writeFileSync(job.output, statement);
            saved = true;
            break;
          }
        }

        await page.waitForTimeout(500);
      }

      if(!saved) {
        throw new Error('No statement (HTML or PDF)');
      }
    } catch(err) {
      failed.push({
        url: job.url,
        error: err.message
      });
    } finally {
      if(page) {
        try {
          await page.close();
        } catch(e) {}
      }
      done++;
      if(done % 25 == 0 || done == jobs.length) {
        console.error("\r" + done + "/" + jobs.length + " (" + Math.round((Date.now() - start) / 1000) + "s)");
      }
    }
  }

  async function processBatch(batch) {
    const browser = await chromium.launch({ headless: false });

    let next = 0;
    const workers = [];
    const n = Math.min(CONCURRENCY, batch.length);
    for(let w = 0; w < n; w++) {
      workers.push((async () => {
        while(true) {
          const job = batch[next++];
          if(!job) {
            return;
          }
          await fetchOne(browser, job);
        }
      })());
    }
    await Promise.all(workers);

    await browser.close();
  }

  for(let i = 0; i < jobs.length; i += BATCH_SIZE) {
    await processBatch(jobs.slice(i, i + BATCH_SIZE));
  }

  if(failed.length > 0) {
    console.error("\n" + failed.length + " failures:");
    for(const f of failed) {
      console.error("  " + f.url + " -> " + f.error);
    }
    process.exit(1);
  }
  console.error("\nDone");
}

main().catch(err => {
  console.error(err);
  process.exit(1);
});