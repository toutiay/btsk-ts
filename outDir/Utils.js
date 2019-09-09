"use strict";
Object.defineProperty(exports, "__esModule", { value: true });
function ASSERT(bool) {
    if (!bool) {
        console.log("ASSERT.bool == " + bool);
    }
}
exports.ASSERT = ASSERT;
function CHECK(X) {
    if (!X) {
        console.log("CHECK.X == " + X);
    }
}
exports.CHECK = CHECK;
function CHECK_EQUAL(X, Y) {
    if (X != Y) {
        console.log("CHECK_EQUAL.(" + X + " == " + Y + ") ");
        console.log("CHECK_EQUAL.expected:" + (X) + " actual:" + (Y) + " ");
    }
}
exports.CHECK_EQUAL = CHECK_EQUAL;
//# sourceMappingURL=Utils.js.map