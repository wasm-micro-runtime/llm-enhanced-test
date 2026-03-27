const fs = require("fs");
const path = require("path");

const INFRA_PREFIXES = [
    "bh_", "_Z", "addr_pool_", "argv_environ_",
    "wasm_runtime_full_init", "wasm_runtime_destroy",
    "wasm_runtime_env_init",
    "wasm_runtime_memory_init", "wasm_runtime_memory_destroy",
    "wasm_runtime_malloc", "wasm_runtime_free",
    "wasm_runtime_malloc_internal", "wasm_runtime_free_internal",
    "wasm_runtime_set_exec_env_tls", "wasm_runtime_get_exec_env_tls",
    "wasm_runtime_detect_native_stack_overflow",
];

const SETUP_TEARDOWN_FUNCS = [
    "wasm_runtime_load", "wasm_runtime_load_ex",
    "wasm_runtime_unload",
    "wasm_runtime_instantiate", "wasm_runtime_instantiate_internal",
    "wasm_runtime_deinstantiate", "wasm_runtime_deinstantiate_internal",
    "wasm_runtime_create_exec_env", "wasm_runtime_destroy_exec_env",
    "wasm_runtime_set_running_mode",
    "bh_read_file_to_buffer",
];

const BRANCH_KEYWORDS = /^\s*(if|else|else\s*if|switch|case|default|return|goto|break|continue|for|while|do)\b/;
const GAP_CONTEXT_LINES = 1;
const MAX_REGIONS_PER_FUNC = 8;

function isInfraFunc(funcName) {
    return INFRA_PREFIXES.some(prefix => funcName.startsWith(prefix));
}

const args = process.argv.slice(2);
let inputPath = null, outputDir = null, testFilePath = null;

for (let i = 0; i < args.length; i++) {
    if (args[i] === "--test-file" && i + 1 < args.length) {
        testFilePath = args[++i];
    } else if (!inputPath) {
        inputPath = args[i];
    } else if (!outputDir) {
        outputDir = args[i];
    }
}

if (!inputPath || !outputDir) {
    console.error("Usage: node filter_coverage.js <input.info> <output_dir> [--test-file <test.cc>]");
    console.error("  Without --test-file: full filtered summary (all files)");
    console.error("  With --test-file:    focused summary (only target functions)");
    process.exit(1);
}

if (!fs.existsSync(outputDir)) {
    fs.mkdirSync(outputDir, { recursive: true });
}

const infoOutputPath = path.join(outputDir, "coverage-simple.info");
const mdOutputPath = path.join(outputDir, "coverage-summary.md");

let targetFuncs = null;

if (testFilePath) {
    targetFuncs = extractTargetFunctions(testFilePath);
    console.log(`Target functions extracted from ${path.basename(testFilePath)}:`);
    console.log(`  Direct API calls: ${targetFuncs.directCalls.join(", ")}`);
    console.log(`  Inferred targets: ${targetFuncs.inferredTargets.join(", ")}`);
    console.log(`  Setup/Teardown:   ${targetFuncs.setupFuncs.join(", ")}`);
}

function extractTargetFunctions(ccFilePath) {
    const content = fs.readFileSync(ccFilePath, "utf-8");
    const lines = content.split("\n");

    const directCalls = new Set();
    const inferredTargets = new Set();
    const setupFuncs = new Set();

    const callRegex = /\b(wasm_runtime_\w+|aot_\w+|bh_\w+)\s*\(/g;
    for (const line of lines) {
        if (line.trim().startsWith("//") || line.trim().startsWith("*")) continue;
        let match;
        while ((match = callRegex.exec(line)) !== null) {
            const funcName = match[1];
            if (SETUP_TEARDOWN_FUNCS.includes(funcName)) {
                setupFuncs.add(funcName);
            } else if (!isInfraFunc(funcName)) {
                directCalls.add(funcName);
            }
        }
    }

    const basename = path.basename(ccFilePath, ".cc").replace("enhanced_", "").replace("_test", "");
    const opPatterns = extractOpPatterns(basename);

    const coverageTargetRegex = /@coverage_target\s+(.+)/g;
    let ctMatch;
    while ((ctMatch = coverageTargetRegex.exec(content)) !== null) {
        const target = ctMatch[1].trim();
        const colonIdx = target.lastIndexOf(":");
        if (colonIdx > 0) {
            inferredTargets.add(target.substring(colonIdx + 1));
        }
    }

    return {
        directCalls: [...directCalls],
        inferredTargets: [...inferredTargets],
        setupFuncs: [...setupFuncs],
        opPatterns,
        allTargets: new Set([...directCalls, ...inferredTargets]),
    };
}

function extractOpPatterns(basename) {
    // f32_const → ["f32_const", "f32.const"]
    const patterns = [basename];
    patterns.push(basename.replace(/_/g, "."));
    const parts = basename.split("_");
    if (parts.length >= 2) {
        // e.g., "f32_const" → match "compile_op_f32_const", "f32_const_operation"
        patterns.push(parts.join("_"));
    }
    return patterns;
}

function isFuncMatchingTarget(funcName, targets) {
    if (!targets) return true;
    if (targets.allTargets.has(funcName)) return true;
    for (const pat of targets.opPatterns) {
        if (funcName.includes(pat)) return true;
    }
    for (const dc of targets.directCalls) {
        // wasm_runtime_lookup_function → wasm_lookup_function
        const stripped = dc.replace("wasm_runtime_", "wasm_");
        if (funcName === stripped || funcName.includes(stripped.replace("wasm_", ""))) return true;
    }
    return false;
}

const coverageInfo = fs.readFileSync(inputPath, "utf-8");
const covLines = coverageInfo.split("\n");

const blocks = [];
const infoLines = [];
let currentBlock = [];
let outputBlocks = 0, skippedBlocks = 0;

for (const line of covLines) {
    if (line.startsWith("SF:")) {
        currentBlock = [line];
    } else if (line === "end_of_record" || line.startsWith("end_of_record")) {
        const sfLine = currentBlock[0];
        const sourceFile = sfLine.substring(3);

        const fnDefs = [];
        const funcs = [];
        const coveredLines = [];
        const allDALines = [];

        for (const l of currentBlock.slice(1)) {
            if (l.startsWith("FN:")) {
                const m = l.match(/^FN:(\d+),(.+)$/);
                if (m) fnDefs.push({ name: m[2], startLine: parseInt(m[1]) });
            } else if (l.startsWith("DA:")) {
                const parts = l.substring(3).split(",");
                allDALines.push({ lineNum: parseInt(parts[0]), count: parseInt(parts[1]) });
            }
        }

        const filteredFNDA = [];
        const filteredDA = [];

        for (const l of currentBlock.slice(1)) {
            if (l.startsWith("FNDA:")) {
                const match = l.match(/^FNDA:(\d+),(.+)$/);
                if (match && match[1] !== "0" && !isInfraFunc(match[2])) {
                    filteredFNDA.push(l);
                    funcs.push({ name: match[2], count: parseInt(match[1]) });
                }
            } else if (l.startsWith("DA:")) {
                const m = l.match(/^DA:(\d+),(\d+)/);
                if (m && m[2] !== "0") {
                    filteredDA.push(l);
                    coveredLines.push(parseInt(m[1]));
                }
            }
        }

        if (filteredFNDA.length > 0) {
            infoLines.push(sfLine);
            for (const l of filteredFNDA) infoLines.push(l);
            for (const l of filteredDA) infoLines.push(l);
            infoLines.push("end_of_record");
            outputBlocks++;

            const gapLines = allDALines.filter(d => d.count === 0).map(d => d.lineNum);

            blocks.push({
                sourceFile, fnDefs, funcs, coveredLines, gapLines,
                totalDA: allDALines.length,
                coveredDA: coveredLines.length,
            });
        } else {
            skippedBlocks++;
        }

        currentBlock = [];
    } else {
        currentBlock.push(line);
    }
}

fs.writeFileSync(infoOutputPath, infoLines.join("\n") + "\n", "utf-8");

const sourceCache = {};
function readSourceFile(filePath) {
    if (sourceCache[filePath] !== undefined) return sourceCache[filePath];
    try {
        sourceCache[filePath] = fs.readFileSync(filePath, "utf-8").split("\n");
    } catch {
        sourceCache[filePath] = null;
    }
    return sourceCache[filePath];
}

function buildFuncRanges(fnDefs, totalLines) {
    const sorted = [...fnDefs].sort((a, b) => a.startLine - b.startLine);
    return sorted.map((f, i) => ({
        name: f.name,
        start: f.startLine,
        end: i + 1 < sorted.length ? sorted[i + 1].startLine - 1 : totalLines,
    }));
}

function findFuncForLine(funcRanges, lineNum) {
    for (const r of funcRanges) {
        if (lineNum >= r.start && lineNum <= r.end) return r.name;
    }
    return null;
}

function groupIntoRegions(sortedLines) {
    if (sortedLines.length === 0) return [];
    const regions = [];
    let start = sortedLines[0], end = sortedLines[0];
    for (let i = 1; i < sortedLines.length; i++) {
        if (sortedLines[i] <= end + 2) {
            end = sortedLines[i];
        } else {
            regions.push({ start, end });
            start = end = sortedLines[i];
        }
    }
    regions.push({ start, end });
    return regions;
}

function extractRegionSnippet(sourceLines, region) {
    const from = Math.max(0, region.start - 1 - GAP_CONTEXT_LINES);
    const to = Math.min(sourceLines.length - 1, region.end - 1 + GAP_CONTEXT_LINES);
    const snippet = [];
    for (let i = from; i <= to; i++) {
        const code = sourceLines[i];
        const trimmed = code.trimStart();
        if (trimmed === "" || trimmed === "{" || trimmed === "}") continue;
        snippet.push({ lineNum: i + 1, code, isBranch: BRANCH_KEYWORDS.test(code) });
    }
    return snippet;
}

function renderFuncGapAnalysis(md, funcName, gaps, block, sourceLines, gapSet) {
    const regions = groupIntoRegions(gaps);
    let rendered = 0;

    for (const region of regions) {
        if (rendered >= MAX_REGIONS_PER_FUNC) break;
        const snippet = extractRegionSnippet(sourceLines, region);
        const hasBranch = snippet.some(s => s.isBranch);
        if (!hasBranch && regions.length > 3) continue;
        rendered++;

        const rangeStr = region.start === region.end
            ? `Line ${region.start}` : `Lines ${region.start}-${region.end}`;
        const tag = hasBranch ? "**BRANCH**" : "plain";

        md.push(`${rangeStr} (${tag}):`);
        md.push("```c");
        for (const s of snippet) {
            const marker = gapSet.has(s.lineNum) ? "!" : " ";
            md.push(`${marker} ${s.lineNum}: ${s.code}`);
        }
        md.push("```");
        md.push("");
    }

    if (rendered >= MAX_REGIONS_PER_FUNC) {
        md.push(`> ... and ${regions.length - MAX_REGIONS_PER_FUNC} more gap regions`);
        md.push("");
    } else if (rendered === 0 && regions.length > 0) {
        md.push(`> ${regions.length} gap regions (no branch keywords, likely sequential code)`);
        md.push("");
    }
}

const md = [];

if (targetFuncs) {
    generateFocusedSummary(md);
} else {
    generateFullSummary(md);
}

fs.writeFileSync(mdOutputPath, md.join("\n"), "utf-8");
console.log(`Output: ${md.length} lines → ${mdOutputPath}`);

function generateFocusedSummary(md) {
    md.push("# Focused Coverage Summary");
    md.push("");
    md.push(`> Test file: \`${path.basename(testFilePath)}\``);
    md.push(`> Coverage: \`${path.basename(inputPath)}\``);
    md.push("");

    const targetHits = [];   // { block, func, funcRange, category }
    const neverCalled = [];  // { block, funcRange, category }

    for (const block of blocks) {
        const sourceLines = readSourceFile(block.sourceFile);
        const totalLines = sourceLines ? sourceLines.length : 99999;
        const funcRanges = buildFuncRanges(block.fnDefs, totalLines);

        for (const fr of funcRanges) {
            if (!isFuncMatchingTarget(fr.name, targetFuncs)) continue;

            const hitFunc = block.funcs.find(f => f.name === fr.name);
            const category = targetFuncs.allTargets.has(fr.name) ? "direct"
                : SETUP_TEARDOWN_FUNCS.includes(fr.name) ? "setup"
                : "inferred";

            if (hitFunc) {
                targetHits.push({ block, func: hitFunc, funcRange: fr, category });
            } else {
                neverCalled.push({ block, funcRange: fr, category });
            }
        }
    }

    // === Section 1: Direct API calls ===
    md.push("## 1. Direct API Functions (called by test code)");
    md.push("");

    const directHits = targetHits.filter(t => t.category === "direct");
    if (directHits.length === 0) {
        md.push("> No direct API functions found in coverage data.");
        md.push("");
    }

    for (const { block, func, funcRange } of directHits) {
        const shortFile = block.sourceFile.replace(/.*wasm-micro-runtime\//, "");
        const sourceLines = readSourceFile(block.sourceFile);
        const gapSet = new Set(block.gapLines);
        const funcGaps = block.gapLines.filter(l => l >= funcRange.start && l <= funcRange.end).sort((a, b) => a - b);
        const funcCovered = block.coveredLines.filter(l => l >= funcRange.start && l <= funcRange.end);

        md.push(`### \`${func.name}\` — ${func.count} calls`);
        md.push(`> ${shortFile} : lines ${funcRange.start}-${funcRange.end}`);
        md.push(`> Covered: ${funcCovered.length} lines | Gaps: ${funcGaps.length} lines`);
        md.push("");

        if (funcGaps.length > 0 && sourceLines) {
            renderFuncGapAnalysis(md, func.name, funcGaps, block, sourceLines, gapSet);
        } else if (funcGaps.length === 0) {
            md.push("> All paths covered within this function.");
            md.push("");
        }
    }

    // === Section 2: Inferred compilation targets ===
    md.push("## 2. Inferred Compilation Targets (from test name / wasm opcode)");
    md.push("");

    const inferredHits = targetHits.filter(t => t.category === "inferred");
    const inferredNever = neverCalled.filter(t => t.category === "inferred" || t.category === "direct");

    for (const { block, func, funcRange } of inferredHits) {
        const shortFile = block.sourceFile.replace(/.*wasm-micro-runtime\//, "");
        const sourceLines = readSourceFile(block.sourceFile);
        const gapSet = new Set(block.gapLines);
        const funcGaps = block.gapLines.filter(l => l >= funcRange.start && l <= funcRange.end).sort((a, b) => a - b);
        const funcCovered = block.coveredLines.filter(l => l >= funcRange.start && l <= funcRange.end);

        md.push(`### \`${func.name}\` — ${func.count} calls`);
        md.push(`> ${shortFile} : lines ${funcRange.start}-${funcRange.end}`);
        md.push(`> Covered: ${funcCovered.length} lines | Gaps: ${funcGaps.length} lines`);
        md.push("");

        if (funcGaps.length > 0 && sourceLines) {
            renderFuncGapAnalysis(md, func.name, funcGaps, block, sourceLines, gapSet);
        } else if (funcGaps.length === 0) {
            md.push("> All paths covered within this function.");
            md.push("");
        }
    }

    // Never called target functions
    if (inferredNever.length > 0) {
        md.push("### Never called target functions");
        md.push("");
        for (const { block, funcRange } of inferredNever) {
            const shortFile = block.sourceFile.replace(/.*wasm-micro-runtime\//, "");
            md.push(`- \`${funcRange.name}\` (${shortFile}:${funcRange.start}-${funcRange.end})`);
        }
        md.push("");
    }

    // === Section 3: Setup/Teardown summary ===
    const setupHits = targetHits.filter(t => t.category === "setup");
    if (setupHits.length > 0) {
        md.push("## 3. Setup/Teardown Functions (brief)");
        md.push("");
        md.push("| Function | Calls | File | Gaps |");
        md.push("|----------|-------|------|------|");
        for (const { block, func, funcRange } of setupHits) {
            const shortFile = block.sourceFile.replace(/.*wasm-micro-runtime\//, "");
            const funcGaps = block.gapLines.filter(l => l >= funcRange.start && l <= funcRange.end);
            md.push(`| \`${func.name}\` | ${func.count} | ${shortFile} | ${funcGaps.length} |`);
        }
        md.push("");
    }

    // === Section 4: Overall stats ===
    md.push("## 4. Overall Coverage Stats");
    md.push("");
    const totalCovered = blocks.reduce((s, b) => s + b.coveredDA, 0);
    const totalDA = blocks.reduce((s, b) => s + b.totalDA, 0);
    md.push(`- Total source files with hits: ${blocks.length}`);
    md.push(`- Total lines covered: ${totalCovered} / ${totalDA} (${(totalCovered / totalDA * 100).toFixed(1)}%)`);
    md.push(`- Target functions analyzed: ${targetHits.length} hit + ${neverCalled.length} never called`);
    md.push("");
}

function generateFullSummary(md) {
    const CORE_PATH_KEYWORDS = ["/compilation/", "/aot/", "/interpreter/", "wasm_loader", "wasm_runtime.c"];
    const INFRA_PATH_KEYWORDS = ["/platform/", "/mem-alloc/", "/libc-wasi/", "posix_", "ems_", "locking.h", "refcount.h"];
    const TOP_N = 10;

    function fileScore(sf) {
        let s = 0;
        for (const k of CORE_PATH_KEYWORDS) if (sf.includes(k)) s += 10;
        for (const k of INFRA_PATH_KEYWORDS) if (sf.includes(k)) s -= 10;
        return s;
    }

    blocks.sort((a, b) => {
        const sa = fileScore(a.sourceFile), sb = fileScore(b.sourceFile);
        if (sa !== sb) return sb - sa;
        return b.funcs.reduce((s, f) => s + f.count, 0) - a.funcs.reduce((s, f) => s + f.count, 0);
    });

    md.push("# Coverage Summary");
    md.push("");
    md.push(`> Filtered from \`${path.basename(inputPath)}\`: **${outputBlocks}** source files kept, **${skippedBlocks}** skipped`);
    md.push("");

    const top = blocks.slice(0, TOP_N);
    const rest = blocks.slice(TOP_N);

    if (top.length > 0) md.push("# Primary Targets (detailed)"), md.push("");

    for (const block of top) {
        const shortFile = block.sourceFile.replace(/.*wasm-micro-runtime\//, "");
        const sourceLines = readSourceFile(block.sourceFile);
        const totalLines = sourceLines ? sourceLines.length : 99999;
        const funcRanges = buildFuncRanges(block.fnDefs, totalLines);
        const rate = block.totalDA > 0 ? ((block.coveredDA / block.totalDA) * 100).toFixed(1) : "N/A";
        const gapSet = new Set(block.gapLines);

        md.push(`## ${shortFile}`);
        md.push(`Line coverage: **${block.coveredDA}/${block.totalDA}** (${rate}%)`);
        md.push("");
        md.push("| Function | Calls | Lines |");
        md.push("|----------|-------|-------|");
        block.funcs.sort((a, b) => b.count - a.count).forEach(f => {
            const r = funcRanges.find(r => r.name === f.name);
            md.push(`| \`${f.name}\` | ${f.count} | ${r ? `${r.start}-${r.end}` : "?"} |`);
        });
        md.push("");

        if (block.gapLines.length > 0 && sourceLines) {
            md.push("### Uncovered paths");
            md.push("");
            const hitNames = new Set(block.funcs.map(f => f.name));
            const gapsByFunc = {};
            for (const l of block.gapLines) {
                const fn = findFuncForLine(funcRanges, l) || "__outside__";
                if (!gapsByFunc[fn]) gapsByFunc[fn] = [];
                gapsByFunc[fn].push(l);
            }
            for (const fn of Object.keys(gapsByFunc)) {
                if (fn === "__outside__") continue;
                const gaps = gapsByFunc[fn].sort((a, b) => a - b);
                if (!hitNames.has(fn) && isInfraFunc(fn)) continue;
                if (!hitNames.has(fn)) {
                    md.push(`#### \`${fn}\` — never called (${gaps.length} gap lines)`);
                    md.push("");
                    continue;
                }
                md.push(`#### \`${fn}\` — ${gaps.length} uncovered lines`);
                md.push("");
                renderFuncGapAnalysis(md, fn, gaps, block, sourceLines, gapSet);
            }
        }
        md.push("---");
        md.push("");
    }

    if (rest.length > 0) {
        md.push("# Secondary Files (summary)");
        md.push("");
        md.push("| Source File | Coverage | Top Function | Calls |");
        md.push("|-------------|----------|-------------|-------|");
        for (const b of rest) {
            const sf = b.sourceFile.replace(/.*wasm-micro-runtime\//, "");
            const rate = b.totalDA > 0 ? ((b.coveredDA / b.totalDA) * 100).toFixed(1) : "N/A";
            const top = b.funcs.sort((a, b) => b.count - a.count)[0];
            md.push(`| ${sf} | ${b.coveredDA}/${b.totalDA} (${rate}%) | \`${top.name}\` | ${top.count} |`);
        }
        md.push("");
    }
}
