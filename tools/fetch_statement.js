const { chromium } = require('playwright');
const fs = require('fs');

const CONCURRENCY = 5;
const NAV_TIMEOUT = 90000;

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

  const browser = await chromium.launch({ headless: false });
  const failed = [];
  let done = 0;
  const start = Date.now();

  async function fetchOne(job) {
    let page = null;
    try {
      page = await browser.newPage();
      await page.goto(job.url, { waitUntil: 'domcontentloaded', timeout: NAV_TIMEOUT });
      await page.waitForSelector('.problem-statement', { timeout: NAV_TIMEOUT });

      const statement = await page.locator('.problem-statement').innerHTML().catch(() => null);
      if(!statement || statement.includes('Just a moment')) {
        throw new Error('Cloudflare challenge or empty statement');
      }

      fs.mkdirSync(require('path').dirname(job.output), { recursive: true });
      fs.writeFileSync(job.output, statement);
    } catch(err) {
      failed.push({
        url: job.url,
        error: err.message
      });
    } finally {
      if(page) {
        await page.close();
      }
      done++;
      if(done % 25 == 0 || done == jobs.length) {
        console.error("\r" + done + "/" + jobs.length + " (" + Math.round((Date.now() - start) / 1000) + "s)");
      }
    }
  }

  let next = 0;
  const workers = [];
  const n = Math.min(CONCURRENCY, jobs.length);
  for(let w = 0; w < n; w++) {
    workers.push((async () => {
      while(true) {
        const job = jobs[next++];
        if(!job) {
          return;
        }
        await fetchOne(job);
      }
    })());
  }
  await Promise.all(workers);
  await browser.close();

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