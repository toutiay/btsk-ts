"use strict";
var __importDefault = (this && this.__importDefault) || function (mod) {
    return (mod && mod.__esModule) ? mod : { "default": mod };
};
Object.defineProperty(exports, "__esModule", { value: true });
const MockBehavior_1 = require("./MockBehavior");
const Composite_1 = require("./../BehaviorTreeShared/Composite");
const MockNode_1 = require("../BehaviorTreeShared/MockNode");
const Test_1 = __importDefault(require("./Test"));
function createClass(fname, ftype) {
    let c = class extends ftype {
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
function createClass1(fname, ftype) {
    let c = class extends Composite_1.Composite {
        constructor(size) {
            super();
            for (let i = 0; i < size; i++) {
                this.m_Children.push(new MockNode_1.MockNode);
            }
        }
        getOperator(index) {
            Test_1.default.ASSERT(index < this.m_Children.length);
            let task = this.m_Children[index].m_pTask;
            Test_1.default.ASSERT(task != null);
            return task;
        }
        create() {
            return new ftype(this);
        }
        destroy(t) {
        }
    };
    return c;
}
exports.createClass1 = createClass1;
//# sourceMappingURL=MockComposite.js.map