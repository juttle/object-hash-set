var Bubo = require('../index');
var _ = require('underscore');
var expect = require('chai').expect;
var util = require('util');

function getAttributeString(point, ignoredAttributes) {
    ignoredAttributes = ignoredAttributes || [];
    var keys = [];
    _.each(point, function(val, key) {
        if (!_.contains(ignoredAttributes, key)) {
            keys.push(key);
        }
    });

    keys.sort();

    var attrString = '';
    var comma = '';
    for (var tag = keys.length - 1; tag >= 0; tag--) {
        attrString = keys[tag] + '=' + point[keys[tag]] + comma + attrString;
        comma = ',';
    }

    return attrString;
}

var result = {};

function add(bubo, point) {
    return bubo.add(point, result);
}

function contains(bubo, point) {
    return bubo.contains(point);
}

var options = {};

describe('bubo', function() {
    var point = {
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

    it('initializes properly', function() {
        var bubo = new Bubo(options);

        expect(bubo.add).is.a.function;

        expect(function() {
            bubo.add();
        }).to.throw('Add: invalid arguments');

    });

    it('runs the c++ unit tests', function() {
        var bubo = new Bubo(options);
        bubo.test();
    });

    it('handles add and remove correctly', function() {
        var bubo = new Bubo(options);

        var expected = getAttributeString(point);
        var is_new = add(bubo, point);
        expect(result.attr_str).equal(expected);
        expect(is_new).to.be.true;

        is_new = add(bubo, point);
        expect(result.attr_str).equal(expected);
        expect(is_new).to.be.false;

        // modify point and add. This should be false, and the attr_atr should differ from expected.
        var point2 = JSON.parse(JSON.stringify(point));
        point2.pop = 'NY';
        is_new = add(bubo, point2);
        expect(result.attr_str).not.equal(expected);
        expect(is_new).to.be.true;
    });

    it('add without result works', function() {
        var bubo = new Bubo(options);
        bubo.add(point);

        expect(bubo.contains(point)).equal(true);
    });

    it('contains: checks if a point has been added without adding it', function() {
        var bubo = new Bubo(options);

        var found = contains(bubo, point);
        expect(found).equal(false);

        found = contains(bubo, point);
        expect(found).equal(false);

        var is_new = add(bubo, point);
        expect(is_new).equal(true);

        found = contains(bubo, point);
        expect(found).equal(true);

        found = contains(bubo, point);
        expect(found).equal(true);
    });

    it('delete: removes a specified point', function() {
        var bubo = new Bubo(options);

        add(bubo, point);

        expect(contains(bubo, point)).equal(true);

        bubo.delete(point);

        expect(contains(bubo, point)).equal(false);
    });

    it('returns appropriate stats', function() {
        var bubo = new Bubo({
            ignoredAttributes: ['time', 'value', 'source_type']
        });

        var s1 = {};
        bubo.stats(s1);
        expect(s1.strings_table.num_tags).equal(0);

        var found = add(bubo, point);
        s1 = {};
        bubo.stats(s1);

        expect(s1.strings_table.num_tags).equal(6); // 9 attributes. ignoring time, value, and source_type, 6.
        expect(s1.strings_table.pop).equal(1);
        expect(s1.attrs_table.attr_entries).equal(1);
        expect(s1.attrs_table.blob_allocated_bytes).equal(20971520); //20MB default size
        expect(s1.attrs_table.blob_used_bytes).equal(13); // 1 byte for size, 6 x 2 bytes since all small numbers.

        var point2 = {
            name: 'apple',
            pop: 'NY',
        };
        found = add(bubo, point2);
        s1 = {};
        bubo.stats(s1);

        expect(s1.strings_table.num_tags).equal(6); // no new tags. should be same.
        expect(s1.strings_table.name).equal(2);
        expect(s1.strings_table.pop).equal(2);
        expect(s1.attrs_table.attr_entries).equal(2);
        expect(s1.attrs_table.blob_allocated_bytes).equal(20971520); //20MB default size
        expect(s1.attrs_table.blob_used_bytes).equal(18); // 1 byte for size + 2 x 2 bytes = 5. already have 13, so total 18.
    });

    it('has an ignoredAttributes per Bubo', function() {
        var ignoredAttributes1 = ['time'];

        var ignoredAttributes2 = ['time', 'pop', 'name'];

        var bubo1 = new Bubo({
            ignoredAttributes: ignoredAttributes1
        });

        var bubo2 = new Bubo({
            ignoredAttributes: ignoredAttributes2
        });

        var expected1 = getAttributeString(point, ignoredAttributes1);
        add(bubo1, point);
        expect(result.attr_str).equal(expected1);

        var expected2 = getAttributeString(point, ignoredAttributes2);
        add(bubo2, point);
        expect(result.attr_str).equal(expected2);
    });

    it('rejects ignoredAttributes that is not an array', function() {
        expect(function() { return new Bubo({ignoredAttributes: []}); }).to.not.throw(Error);
        expect(function() { return new Bubo({ignoredAttributes: 1}); }).to.throw(Error);
        expect(function() { return new Bubo({ignoredAttributes: "ignoreme"}); }).to.throw(Error);
        expect(function() { return new Bubo({ignoredAttributes: {}}); }).to.throw(Error);
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
            add(bubo, pt);
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
            add(bubo, point);
            throw new Error('add should have failed');
        } catch (err) {
            expect(err.message).equal('point too big');
        }
    });
});
