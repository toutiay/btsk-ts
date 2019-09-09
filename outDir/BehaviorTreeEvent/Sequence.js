"use strict";
Object.defineProperty(exports, "__esModule", { value: true });
const Composite_1 = require("./Composite");
const Enum_1 = require("../Enum");
const Utils_1 = require("../Utils");
class Sequence extends Composite_1.Composite {
    constructor(bt) {
        super();
        this.m_pBehaviorTree = bt;
        this.m_CurrentIndex = 0;
        this.m_Current = Enum_1.Default.UNDEFINED;
    }
    onInitialize() {
        this.m_CurrentIndex = 0;
        this.m_Current = this.m_Children[this.m_CurrentIndex];
        let observer = this.onChildComplete.bind(this);
        this.m_pBehaviorTree.start(this.m_Current, observer);
    }
    onChildComplete() {
        let child = this.m_Current;
        if (child.m_eStatus == Enum_1.Status.BH_FAILURE) {
            this.m_pBehaviorTree.stop(this, Enum_1.Status.BH_FAILURE);
            return;
        }
        Utils_1.ASSERT(child.m_eStatus == Enum_1.Status.BH_SUCCESS);
        this.m_Current = this.m_Children[++this.m_CurrentIndex];
        if (this.m_Current == null) {
            this.m_pBehaviorTree.stop(this, Enum_1.Status.BH_SUCCESS);
        }
        else {
            let observer = this.onChildComplete.bind(this);
            this.m_pBehaviorTree.start(this.m_Current, observer);
        }
    }
    update() {
        return Enum_1.Status.BH_RUNNING;
    }
}
exports.Sequence = Sequence;
//# sourceMappingURL=Sequence.js.map