#pragma once

void testall();

struct HashToOne {
    uint32_t operator()(const BYTE* b, int len) const;
};
