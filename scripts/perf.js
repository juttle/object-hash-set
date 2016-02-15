var minimist = require('minimist');

var options = minimist(process.argv.slice(2));
var NUM_KEYS = options.num_keys || 3;
var VALUES_PER_KEY = options.values_per_key || 10;

var num_points = Math.pow(VALUES_PER_KEY, NUM_KEYS);
var logging_interval = num_points / 100;

var Bubo = require('../index');
var bubo = new Bubo();

var values = [];
function boundedInt(max) {
    var value = 0;

    return {
        value: function() { return value; },
        increment: function() {
            if (++value % max === 0) {
                value = 0;
            }
        }
    };
}

for (var k = 0; k < NUM_KEYS; k++) {
    values.push(boundedInt(VALUES_PER_KEY));
}

function next_value(point_number, key_number) {
    if (point_number && point_number % Math.pow(VALUES_PER_KEY,key_number) === 0) {
        values[key_number].increment();
    }

    return values[key_number].value();
}

var time = Date.now();
for (var i = 0; i < num_points; i++) {
    if (i % logging_interval === 0) {
        console.log('stored %d points so far in %d sec, memory usage:', i, (Date.now() - time)/1000,process.memoryUsage());
    }

    var point = {};
    for (var j = 0; j < NUM_KEYS; j++) {
        point['key'+j] = 'value' + next_value(i, j);
    }

    bubo.add(point);
}

global.gc();
console.log('Finished! Stored', i, 'points, final memory usage:', process.memoryUsage());
