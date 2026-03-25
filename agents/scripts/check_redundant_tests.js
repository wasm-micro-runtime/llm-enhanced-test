const { readFileSync, writeFileSync, appendFileSync } = require("fs");
const path = require("path");

const moduleName = process.argv[2];
const testFilePath = process.argv[3];
const outputFile = `/tmp/${moduleName}_redundant_check.md`;
const usefulCoverageFile = `/tmp/useful_coverage_info_files.txt`;

const usefulCoverages = readFileSync(usefulCoverageFile, "utf-8")
    .split("\n")
    .map((line) => line.trim())
    .filter(Boolean)
    .map((line) => {
        const m = line.match(/^coverage-(.+)\.info$/);
        return m ? m[1] : line;
    });

/**
 * Extract test case names from a .cc test file.
 * Matches TEST_F(SuiteName, TestName), TEST_P(SuiteName, TestName), TEST(SuiteName, TestName).
 * Returns array of "SuiteName.TestName" (same order as in file, suitable for ctest -R).
 * @param {string} ccFilePath - Path to the .cc file (absolute or relative to cwd)
 * @returns {string[]} e.g. ["F32ConstTest.BasicConstants_ReturnsCorrectValues", "EnhancedAotRuntimeTest.aot_resolve_import_func_..."]
 */
function getTestNamesFromSource(ccFilePath) {
    const fullPath = path.isAbsolute(ccFilePath) ? ccFilePath : path.join(process.cwd(), ccFilePath);
    const content = readFileSync(fullPath, "utf-8");
    const names = [];
    const re = /TEST(?:_F|_P)?\s*\(\s*(\w+)\s*,\s*(\w+)\s*\)/g;
    let m;
    while ((m = re.exec(content)) !== null) {
        names.push(`${m[1]}.${m[2]}`);
    }
    return names;
}

let baseNames = getTestNamesFromSource(testFilePath);
const totalTestCount = baseNames.length;

const usefulTestCases = [];
const redundantTestCases = [];
    
for (const testCase of baseNames) {
    for (const usefulCoverage of usefulCoverages) {
        if (testCase.includes(usefulCoverage)) {
            usefulTestCases.push(testCase);
            break;
        }
    }
    if (!usefulTestCases.includes(testCase)) {
        redundantTestCases.push(testCase);
    }
}

writeFileSync(outputFile, `# Redundant Test Check Report\n\n`);
appendFileSync(outputFile, `## Summary\n`);

appendFileSync(outputFile, `- **Total tests:** ${totalTestCount}\n`);
appendFileSync(outputFile, `- **Useful tests:** ${usefulTestCases.length}\n`);
appendFileSync(outputFile, `- **Redundant tests:** ${redundantTestCases.length}\n`);
appendFileSync(outputFile, `\n`);

if (redundantTestCases.length > 0) {
    appendFileSync(outputFile, `## Redundant Tests (suggest to delete)\n`);
    for (const testCase of redundantTestCases) {
        appendFileSync(outputFile, `- ❌ ${testCase}\n`);
    }
}

if (usefulTestCases.length > 0) {
    appendFileSync(outputFile, `## Useful Tests (suggest to keep)\n`);
    for (const testCase of usefulTestCases) {
        appendFileSync(outputFile, `- ✅ ${testCase}\n`);
    }
}

console.log(`========================================`);

console.log(`detection completed, check the report: ${outputFile}`);
console.log(`========================================`);
