"use strict";
Object.defineProperty(exports, "__esModule", { value: true });
const MockBehavior_1 = require("./MockBehavior");
function createClass(fname, COMPOSITE) {
    let c = class extends COMPOSITE {
        constructor(size) {
            super();
            for (let i = 0; i < size; i++) {
                this.m_Children.push(new MockBehavior_1.MockBehavior);
            }
        }
        getOperator(index) {
            return this.m_Children[index];
        }
    };
    return c;
}
exports.createClass = createClass;
//# sourceMappingURL=MockComposite.js.map