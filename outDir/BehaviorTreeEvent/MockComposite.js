"use strict";
Object.defineProperty(exports, "__esModule", { value: true });
const MockBehavior_1 = require("./MockBehavior");
const Utils_1 = require("../Utils");
function createClass(fname, COMPOSITE) {
    let c = class extends COMPOSITE {
        constructor(bt, size) {
            super(bt);
            for (let i = 0; i < size; i++) {
                this.m_Children.push(new MockBehavior_1.MockBehavior);
            }
        }
        getOperator(index) {
            Utils_1.ASSERT(index < this.m_Children.length);
            return this.m_Children[index];
        }
    };
    return c;
}
exports.createClass = createClass;
//# sourceMappingURL=MockComposite.js.map