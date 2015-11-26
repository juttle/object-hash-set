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

function contains(bubo, bucket, point) {
    return bubo.contains(bucket, point);
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

    it('contains: checks if a point has been added without adding it', function() {
        var bubo = new Bubo(options);
        var bucket = 'contains_bucket';

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

        var found = contains(bubo, bucket, pt);
        expect(found).equal(false);

        found = contains(bubo, bucket, pt);
        expect(found).equal(false);

        found = add(bubo, bucket, pt);
        expect(found).equal(false);

        found = contains(bubo, bucket, pt);
        expect(found).equal(true);

        found = contains(bubo, bucket, pt);
        expect(found).equal(true);
    });

    it('delete: removes a specified point', function() {
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

        var bucket = 'delete_test';

        add(bubo, bucket, pt);

        expect(contains(bubo, bucket, pt)).equal(true);

        bubo.delete('delete_test', pt);

        expect(contains(bubo, bucket, pt)).equal(false);
    });

    it('get_buckets', function() {
        var bubo = new Bubo(options);

        var initial_buckets = bubo.get_buckets();
        expect(initial_buckets).deep.equal([]);

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

        var bucket1 = 'bucket1';
        var bucket2 = 'bucket2';
        var bucket3 = 'bucket3';

        add(bubo, bucket1, pt);
        add(bubo, bucket2, pt);
        add(bubo, bucket3, pt);

        var buckets = bubo.get_buckets().sort();

        expect(buckets).deep.equal([bucket1, bucket2, bucket3]);
    });

    it('handles delete_bucket correctly', function() {
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

        var bucket = 'delete_this_bucket';

        var found = add(bubo, bucket, pt);
        expect(found).to.be.false;
        expect(contains(bubo, bucket, pt)).equal(true);

        bubo.delete_bucket(bucket);
        expect(contains(bubo, bucket, pt)).equal(false);
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

    it('has an ignoredAttributes per Bubo', function() {
        var bucket = 'ignored_attributes_bucket';
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

        var ignoredAttributes1 = {
            time: true
        };

        var ignoredAttributes2 = {
            time: true,
            pop: true,
            name: true
        };

        var bubo1 = new Bubo({
            ignoredAttributes: ignoredAttributes1
        });

        var bubo2 = new Bubo({
            ignoredAttributes: ignoredAttributes2
        });

        var expected1 = getAttributeString(pt, ignoredAttributes1);
        add(bubo1, bucket, pt);
        expect(result.attr_str).equal(expected1);

        var expected2 = getAttributeString(pt, ignoredAttributes2);
        add(bubo2, bucket, pt);
        expect(result.attr_str).equal(expected2);
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
