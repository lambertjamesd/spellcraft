const fs = require('fs');

const call_pairings = [
    ['malloc', 'free'],
];

const pair_mapping = call_pairings.reduce((result, curr, index) => {
    result.set(curr[0], {index, offset: 1});
    result.set(curr[1], {index, offset: -1});
    return result;
}, new Map());

const token_search = /(\w+)[\s\n]*\(/gim;

function search_file(contents) {
    let match = token_search.exec(contents);

    const result = [];

    while (result.length < call_pairings.length) {
        result.push({score: 0, match_locations: []});
    }

    while (match) {
        const mapping = pair_mapping.get(match[1]);

        if (mapping) {
            const result_entry = result[mapping.index];
            result_entry.score += mapping.offset;
            result_entry.match_locations.push({
                match: match[1],
                index: match.index
            });
        }
        
        match = token_search.exec(contents);
    }

    return result.filter((entry) => entry.score != 0);
}

function find_line(contents, location) {
    let line = 1;
    let col = 1;

    for (let i = 0; i < location; i += 1) {
        if (contents[i] == '\n') {
            line += 1;
            col = 1;
        } else {
            col += 1;
        }
    }

    return {line, col};
}

function extract_source(contents, location) {
    const before = contents.lastIndexOf('\n', location);
    const after = contents.indexOf('\n', location);

    return contents.substring(before + 1, after === -1 ? contents.length : after);
}

function process_file(filename) {
    return new Promise((resolve) => {
        fs.readFile(filename, 'utf8', (err, data) => {
            if (err) {
                console.error(`failed to open file ${filename}: ${err.message}`);
                resolve(false);
                return;
            }

            const bad_pairings = search_file(data);

            if (bad_pairings.length == 0) {
                resolve(true);
                return;
            }

            console.error(`file ${filename} has mismatched pairings
${bad_pairings.map((pairing) => '    pairings\n' + pairing.match_locations.map((match) => {
    const location = find_line(data, match.index);
    return `        ${match.match} ${filename}:${location.line}\n            ${extract_source(data, match.index)}`
}).join('\n')).join('\n')}`);

            resolve(false);
        });
    });
}

function process_files(files) {
    return Promise.all(files.map(process_file)).then((results) => results.every(a => a));
}

process_files(process.argv.slice(2)).then((result) => {
    if (~result) {
        process.exit(1);
    } else {
        console.log('Good to go!');
    }
});