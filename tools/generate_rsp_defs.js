const fs = require('fs');
const path = require('path');

const objdumpFilename = process.argv[2];
const mnFilename = process.argv[3];
const outputFilename = process.argv[4];

const dataRegex = /a400[0-9a-f]{4} ([0-9a-f]{8}) ([0-9a-f]{8}) ([0-9a-f]{8}) ([0-9a-f]{8})/gim;

const readObjDump = () => {
    const contents = fs.readFileSync(objdumpFilename, 'utf-8');
    const result = [];

    while (true) {
        const match = dataRegex.exec(contents);

        if (!match) {
            break;
        }

        result.push(
            Buffer.from(match[1], 'hex'),
            Buffer.from(match[2], 'hex'),
            Buffer.from(match[3], 'hex'),
            Buffer.from(match[4], 'hex')
        );
    }

    return Buffer.concat(result);
}

const mnRegex = /f{8}a400([0-9a-f]{4})\s+(\w)\s+([\w\d]+)/gim;

const lookupSymbol = (symbols, name) => symbols.find((el) => el.label === name);
const lookupLabel = (symbols, offset) => symbols.find((el) => el.offset === offset && el.type === 't');

const readMn = () => {
    const contents = fs.readFileSync(mnFilename, 'utf-8');

    const result = [];

    while (true) {
        const match = mnRegex.exec(contents);

        if (!match) {
            break;
        }

        result.push({offset: Number.parseInt(match[1].slice(1), 16), type: match[2], label: match[3]});
    }

    return result;
}

const objDumpContents = readObjDump();
const mnContents = readMn();

const savedStart = lookupSymbol(mnContents, '_RSPQ_SAVED_STATE_START');
const savedEnd = lookupSymbol(mnContents, '_RSPQ_SAVED_STATE_END');

console.log(savedEnd, mnContents.find((el) => el.label === 'COLOR_TABLE'));

const savedSymbols = mnContents.filter((el) => el.type === 'd' && 
    el.offset >= savedStart.offset && 
    el.offset < savedEnd.offset && 
    el.label !== '_RSPQ_SAVED_STATE_START'
);

savedSymbols.sort((a, b) => a.offset - b.offset);

const findCommands = () => {
    const commandTable = lookupSymbol(mnContents, '_RSPQ_OVERLAY_COMMAND_TABLE');
    let curr = commandTable.offset;

    const result = [];

    while (true) {
        const cmdAndUpperAddr = objDumpContents[curr];
        const address = (objDumpContents[curr + 1] << 2) | ((cmdAndUpperAddr & 0x3) << 10);

        const cmdCount = cmdAndUpperAddr & 0xFC;

        if (!cmdCount) {
            break;
        }

        const commandLabel = lookupLabel(mnContents, address);

        console.log(`Found command ${commandLabel.label} at 0x${address.toString(16)} mem addr 0x${curr.toString(16)} with count ${cmdCount}`);

        result.push(commandLabel.label);

        curr += 2;
    }

    return result;
}

const commands = findCommands();

const rspName = path.basename(outputFilename, '.h').slice(0, -5);
const rspNameUpper = rspName.toUpperCase();

const formatSymbol = (symbol) => `#define ${rspNameUpper}_${symbol.label} 0x${(symbol.offset - savedStart.offset).toString(16)}`;
const formatCommand = (command, index) => `#define ${rspNameUpper}_${command} ${index}`;

fs.writeFileSync(outputFilename, `#ifndef __${rspNameUpper}_H__
#define __${rspNameUpper}_H__

${commands.map(formatCommand).join('\n')}

#define ${rspNameUpper}_SAVED_START 0x${savedStart.offset.toString(16)}

${savedSymbols.map(formatSymbol).join('\n')}

#endif
`);