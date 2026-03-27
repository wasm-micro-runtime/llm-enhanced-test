const fs = require("fs");
const path = require("path");

const testFilePath = process.argv[2];
const caseName = process.argv[3];

const content = fs.readFileSync(testFilePath, "utf-8");
const lines = content.split("\n");

// Case name may be "TestName" or "SuiteName.TestName"; we match the test name (second arg of macro).
const testName = caseName.includes(".") ? caseName.split(".").pop() : caseName;
const escaped = testName.replace(/[.*+?^${}()|[\]\\]/g, "\\$&");
const re = new RegExp(`TEST(?:_F|_P)?\\s*\\(\\s*\\w+\\s*,\\s*${escaped}\\s*\\)`);

for (let i = 0; i < lines.length; i++) {
    if (re.test(lines[i])) {
        console.log(i + 1);
        process.exit(0);
    }
}

process.exit(1);
