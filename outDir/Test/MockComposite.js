"use strict";
Object.defineProperty(exports, "__esModule", { value: true });
const MockBehavior_1 = require("./MockBehavior");
// export class MockSelector extends Selector {
//     constructor(size: number) {
//         super();
//         createSize(this, size);
//     }
//     getOperator(index: number): MockBehavior {
//         return getMock(this, index);
//     }
// }
// export class MockSequence extends Sequence {
//     constructor(size: number) {
//         super();
//         createSize(this, size);
//     }
//     getOperator(index: number): MockBehavior {
//         return getMock(this, index);
//     }
// }
// export class MockActiveSelector extends ActiveSelector {
//     constructor(size: number) {
//         super();
//         createSize(this, size);
//     }
//     getOperator(index: number): MockBehavior {
//         return getMock(this, index);
//     }
// }
function createInstance(fname, ftype) {
    let c = class extends ftype {
        constructor(size) {
            super();
            createSize(this, size);
        }
        getOperator(index) {
            return getMock(this, index);
        }
    };
    return c;
}
exports.createInstance = createInstance;
function createSize(target, size) {
    for (let i = 0; i < size; i++) {
        target.m_Children.push(new MockBehavior_1.MockBehavior);
    }
}
function getMock(target, index) {
    return target.m_Children[index];
}
//# sourceMappingURL=MockComposite.js.map