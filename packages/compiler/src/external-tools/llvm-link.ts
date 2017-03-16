import * as path from "path";
import * as fs from "fs";
import * as ts from "typescript";
import * as assert from "assert";
import * as debug from "debug";
import { execLLVM } from "./tools";
import { OBJECT_FILES_LOCATION, COMPILER_RT_FILE } from "speedyjs-runtime";
import {BuildDirectory} from "../code-generation/build-directory";
import {LLVMByteCodeSymbolsResolver} from "./llvm-nm";

const EXECUTABLE_NAME = "llvm-link";
const LOG = debug("LLVMLink");

export class LLVMLink {

    private byteCodeFiles: string[] = [];
    private byteCodeSymbolResolver = new LLVMByteCodeSymbolsResolver();

    /**
     * @param buildDirectory the build directory
     */
    constructor(private buildDirectory: BuildDirectory) {

    }

    /**
     * Adds a file that is to be linked
     * @param file the file to be linked
     */
    addByteCodeFile(file: string) {
        assert(ts.sys.fileExists(file), `The file ${file} to link does not exist.`);

        this.byteCodeFiles.push(file);
    }

    addRuntime() {
        for (const file of fs.readdirSync(OBJECT_FILES_LOCATION)) {
            const fullPath = path.join(OBJECT_FILES_LOCATION, file);

            if (fullPath === COMPILER_RT_FILE) {
                continue;
            }

            if (file.endsWith(".a")) {
                this.byteCodeFiles.push(...this.extractArchive(fullPath));
            } else if (file.endsWith(".bc") || file.endsWith(".o")) {
                this.byteCodeFiles.push(fullPath);
            }
        }
    }

    /**
     * Links the files together
     * @param target the path to the file where the linked output should be written to
     * @param entrySymbols the name of the external visible entry symbols
     */
    link(target: string, entrySymbols: string[]): string {
        const linkingFiles = Array.from(this.getObjectFilesToLink(this.byteCodeFiles, entrySymbols));

        const quotedFileNames = Array.from(linkingFiles).map(file => `"${file}"`).join(" ");
        execLLVM(EXECUTABLE_NAME, `${quotedFileNames} -o "${target}"`);
    }

    private extractArchive(archiveFilePath: string): string[] {
        const directory  = this.buildDirectory.getTempSubdirectory(path.basename(archiveFilePath));
        LOG(`Extract archive ${archiveFilePath} to ${directory}`);

        execLLVM("llvm-ar", `x "${archiveFilePath}"`, directory);

        return fs.readdirSync(directory).map(file => path.join(directory, file));
    }

    private getObjectFilesToLink(objectFiles: string[], entrySymbols: string[]) {
        let loopAgain = true;
        let includedObjectFiles = new Set<string>();
        let unresolvedSymbols = new Set<string>(entrySymbols);

        while (loopAgain) {
            loopAgain = false;

            for (const objectFile of objectFiles) {
                if (includedObjectFiles.has(objectFile)) {
                    continue;
                }

                if (this.considerObjectFile(objectFile, unresolvedSymbols)) {
                    includedObjectFiles.add(objectFile);
                    loopAgain = true;
                }
            }
        }

        return includedObjectFiles.values();
    }

    private considerObjectFile(objectFile: string, unresolvedSymbols: Set<string>) {
        const symbols = this.byteCodeSymbolResolver.getSymbols(objectFile);

        LOG(`Consider object file ${objectFile} for linkage`);
        if (intersects(unresolvedSymbols, symbols.defined)) {
            LOG(`Add object file ${objectFile} to linkage`);
            symbols.defined.forEach(symbol => unresolvedSymbols.delete(symbol));
            symbols.undefined.forEach(symbol => unresolvedSymbols.add(symbol));

            return true;
        }

        return false;
    }
}


function  intersects<T>(setA: Set<T>, setB: Set<T>): boolean {
    const smaller = setA.size < setB.size ? setA : setB;
    const larger = setA === smaller ? setB : setA;

    for (const symbol of Array.from(smaller)) {
        if (larger.has(symbol)) {
            return true;
        }
    }

    return false;
}