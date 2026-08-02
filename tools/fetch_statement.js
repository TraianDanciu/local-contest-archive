const { chromium } = require('playwright');
const fs = require('fs');

async function main() {
  if(process.argv.length != 4) {
    console.error("Usage: node fetch_statement.js <url> <output>");
    process.exit(1);
  }

  const url = process.argv[2];
  const output = process.argv[3];

  const browser = await chromium.launch({
    headless: false
  });

  const page = await browser.newPage();

  await page.goto(url, {
    waitUntil: 'domcontentloaded',
    timeout: 60000
  });

  await page.waitForSelector(".problem-statement", {
    timeout: 60000
  });

  const statement = await page.locator(".problem-statement").evaluate(
    node => node.outerHTML
  );

  fs.writeFileSync(output, statement);

  await browser.close();
}

main().catch(err => {
  console.error(err);
  process.exit(1);
})