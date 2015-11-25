var Bubo = require('../index');
var _ = require('underscore');
var expect = require('chai').expect;
var util = require('util');

function getAttributeString(pt, ignoredAttributes) {
    ignoredAttributes = ignoredAttributes || {};
    var keys = [];
    _.each(pt, function(val, key) {
        if (!ignoredAttributes[key]) {
            keys.push(key);
        }
    });

    keys.sort();

    var attrString = '';
    var comma = '';
    for (var tag = keys.length - 1; tag >= 0; tag--) {
        attrString = keys[tag] + '=' + pt[keys[tag]] + comma + attrString;
        comma = ',';
    }

    return attrString;
}

var result = {};

function add(bubo, bucket, point) {
    return bubo.add(bucket, point, result);
}

var options = {}

describe('bubo', function() {
    it('initializes properly', function() {
        var bubo = new Bubo(options);

        expect(bubo.add).is.a.function;

        expect(function() {
            bubo.add({});
        }).to.throw('Add: invalid arguments');

    });

    it('runs the c++ unit tests', function() {
        var bubo = new Bubo(options);
        bubo.test();
    });

    it('handles add and remove correctly', function() {
        var bubo = new Bubo(options);

        var pt = {
            name: 'cpu.system',
            pop: 'SF',
            host: 'foo.com',
            time: new Date(),
            time2: new Date(),
            value: 100,
            value2: 100,
            value3: 100.999,
            source_type: 'metric',
        };

        var expected = getAttributeString(pt);

        // add in different space-buckets combinations.
        // The 'found' should be false when the space-bucket is called first time.

        var found = add(bubo, 'aaa', pt);
        expect(result.attr_str).equal(expected);
        expect(found).to.be.false;

        found = add(bubo, 'bbb', pt);
        expect(result.attr_str).equal(expected);
        expect(found).to.be.false;

        found = add(bubo, 'ccc', pt);
        expect(result.attr_str).equal(expected);
        expect(found).to.be.false;

        // The same calls should have found == true the second time.
        found = add(bubo, 'aaa', pt);
        expect(result.attr_str).equal(expected);
        expect(found).to.be.true;

        found = add(bubo, 'bbb', pt);
        expect(result.attr_str).equal(expected);
        expect(found).to.be.true;

        found = add(bubo, 'ccc', pt);
        expect(result.attr_str).equal(expected);
        expect(found).to.be.true;

        // modify pt and add. This should be false, and the attr_atr should differ from expected.
        var pt2 = JSON.parse(JSON.stringify(pt));
        pt2.pop = 'NY';
        found = add(bubo, 'aaa', pt2);
        expect(result.attr_str).not.equal(expected);
        expect(found).to.be.false;

    });

    it('handles remove_bucket correctly', function() {
        var bubo = new Bubo(options);

        var pt = {
            name: 'cpu.system',
            pop: 'SF',
            host: 'foo.com',
            time: new Date(),
            time2: new Date(),
            value: 100,
            value2: 100,
            value3: 100.999,
            source_type: 'metric',
        };

        var v;

        /*
        Create the point in these space-bucket combinations by performing a add:
            s1,12
            s1,22
            s1,32
            s1,42
            s1.52
            s2,42
            s3,52
        First time all are not found.

        Now, remove_bucket(s1, 12), and add all again.
        All but (s1,12) should be true.

        Now, remove_space(s1,32), and add again.
        Since all buckets <= 32 should have been removed, and hence should be false.
        */

        found = add(bubo, 's1@12', pt);
        expect(found).to.be.false;
        found = add(bubo, 's1@22', pt);
        expect(found).to.be.false;
        found = add(bubo, 's1@32', pt);
        expect(found).to.be.false;
        found = add(bubo, 's1@42', pt);
        expect(found).to.be.false;
        found = add(bubo, 's1@52', pt);
        expect(found).to.be.false;
        found = add(bubo, 's2@42', pt);
        expect(found).to.be.false;
        found = add(bubo, 's3@52', pt);
        expect(found).to.be.false;

        bubo.remove_bucket('s1@12');

        found = add(bubo, 's1@12', pt);
        expect(found).to.be.false;
        found = add(bubo, 's1@22', pt);
        expect(found).to.be.true;
        found = add(bubo, 's1@32', pt);
        expect(found).to.be.true;
        found = add(bubo, 's1@42', pt);
        expect(found).to.be.true;
        found = add(bubo, 's1@52', pt);
        expect(found).to.be.true;
        found = add(bubo, 's2@42', pt);
        expect(found).to.be.true;
        found = add(bubo, 's3@52', pt);
        expect(found).to.be.true;

        // Use integer second param to ensure both integer and string are accepted.
        bubo.remove_bucket('s1@32');

        found = add(bubo, 's1@12', pt);
        expect(found).to.be.false;
        found = add(bubo, 's1@22', pt);
        expect(found).to.be.false;
        found = add(bubo, 's1@32', pt);
        expect(found).to.be.false;
        found = add(bubo, 's1@42', pt);
        expect(found).to.be.true;
        found = add(bubo, 's1@52', pt);
        expect(found).to.be.true;
        found = add(bubo, 's2@42', pt);
        expect(found).to.be.true;
        found = add(bubo, 's3@52', pt);
        expect(found).to.be.true;
    });

    it('returns appropriate stats', function() {
        var bubo = new Bubo({
            ignoredAttributes: {
                time: true,
                value: true,
                source_type: true
            }
        });

        var s1 = {};
        bubo.stats(s1);
        expect(s1.strings_table.num_tags).equal(0);

        // [1]
        var pt = {
            name: 'cpu.system',
            pop: 'SF',
            host: 'foo.com',
            time: new Date(),
            time2: new Date(),
            value: 100,
            value2: 100,
            value3: 100.999,
            source_type: 'metric'
        };

        var found = add(bubo, 'spc@bkt', pt);
        s1 = {};
        bubo.stats(s1);

        expect(s1.strings_table.num_tags).equal(6); // 9 attributes. ignoring time, value, and source_type, 6.
        expect(s1.strings_table.pop).equal(1);
        expect(s1['spc@bkt'].attr_entries).equal(1);
        expect(s1['spc@bkt'].blob_allocated_bytes).equal(20971520); //20MB default size
        expect(s1['spc@bkt'].blob_used_bytes).equal(13); // 1 byte for size, 6 x 2 bytes since all small numbers.

        // [2] another point that reuses some strings.
        pt = {
            name: 'apple',
            pop: 'NY',
        };
        found = add(bubo, 'spc@bkt', pt);
        s1 = {};
        bubo.stats(s1);

        expect(s1.strings_table.num_tags).equal(6); // no new tags. should be same.
        expect(s1.strings_table.name).equal(2);
        expect(s1.strings_table.pop).equal(2);
        expect(s1['spc@bkt'].attr_entries).equal(2);
        expect(s1['spc@bkt'].blob_allocated_bytes).equal(20971520); //20MB default size
        expect(s1['spc@bkt'].blob_used_bytes).equal(18); // 1 byte for size + 2 x 2 bytes = 5. already have 13, so total 18.
    });

    it.skip('profiles the memory use of adding 7 million points', function() {
        this.timeout(900000);
        var bubo = new Bubo(options);

        var pt = {
            space: 'default',
            name: 'cpu.system',
            pop: 'SF',
            host: 'foo.com',
            time: new Date(),
            time2: new Date(),
            value: 100,
            value2: 100,
            value3: 100.999,
            source_type: 'metric',
        };

        var s1 = {};
        console.log('js:  ' + util.inspect(process.memoryUsage()));
        for (var i = 0; i < 7000000; i++) {
            pt.pop = 'SF' + i;
            pt.value2 = pt.value2 + (2* i);
            add(bubo, 'test-space-1@421', pt);
            if (i % 1000000 === 0) {
                s1 = {};
                bubo.stats(s1);
                console.log('[' + i + ']' + JSON.stringify(s1, null, 4));
            }
        }
        console.log('js:  ' + util.inspect(process.memoryUsage()));

        s1 = {};
        bubo.stats(s1);
        console.log(JSON.stringify(s1, null, 4));

    });

    it('throws an error looking up a big point', function() {
        var bubo = new Bubo(options);
        var too_big_string = '';

        for (var k = 0; k < 5000; k++) {
            too_big_string += 'dave rules!! ';
        }

        var point = {
            source_type: 'metric',
            space: 'default',
            value: 1,
            time: Date.now(),
            too_big: too_big_string
        };

        try {
            add(bubo, 'big@data', point);
            throw new Error('add should have failed');
        } catch (err) {
            expect(err.message).equal('point too big');
        }
    });
});
