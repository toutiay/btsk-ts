"use strict";
Object.defineProperty(exports, "__esModule", { value: true });
const Selector_1 = require("../BehaviorTree/Selector");
const MockBehavior_1 = require("./MockBehavior");
const Sequence_1 = require("../BehaviorTree/Sequence");
const ActiveSelector_1 = require("../BehaviorTree/ActiveSelector");
class MockSelector extends Selector_1.Selector {
    constructor(size) {
        super();
        createSize(this, size);
    }
    getOperator(index) {
        return getMock(this, index);
    }
}
exports.MockSelector = MockSelector;
class MockSequence extends Sequence_1.Sequence {
    constructor(size) {
        super();
        createSize(this, size);
    }
    getOperator(index) {
        return getMock(this, index);
    }
}
exports.MockSequence = MockSequence;
class MockActiveSelector extends ActiveSelector_1.ActiveSelector {
    constructor(size) {
        super();
        createSize(this, size);
    }
    getOperator(index) {
        return getMock(this, index);
    }
}
exports.MockActiveSelector = MockActiveSelector;
function createSize(target, size) {
    for (let i = 0; i < size; i++) {
        target.m_Children.push(new MockBehavior_1.MockBehavior);
    }
}
function getMock(target, index) {
    return target.m_Children[index];
}
//# sourceMappingURL=MockComposite.js.map