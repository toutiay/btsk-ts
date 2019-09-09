export function ASSERT(bool: boolean) {
    if (!bool) {
        console.log("ASSERT.bool == " + bool);
    }
}

export function CHECK(X: number) {
    if (!X) {
        console.log("CHECK.X == " + X);
    }
}

export function CHECK_EQUAL(X: number, Y: number) {
    if (X != Y) {
        console.log("CHECK_EQUAL.(" + X + " == " + Y + ") ");
        console.log("CHECK_EQUAL.expected:" + (X) + " actual:" + (Y) + " ");
    }
}