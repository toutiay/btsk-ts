"use strict";
Object.defineProperty(exports, "__esModule", { value: true });
const Composite_1 = require("./Composite");
const Default_1 = require("../Enum/Default");
const Status_1 = require("../Enum/Status");
const Test_1 = require("../Test/Test");
class Sequence extends Composite_1.Composite {
    constructor(bt) {
        super();
        this.m_pBehaviorTree = bt;
        this.m_CurrentIndex = 0;
        this.m_Current = Default_1.Default.UNDEFINED;
    }
    onInitialize() {
        this.m_CurrentIndex = 0;
        this.m_Current = this.m_Children[this.m_CurrentIndex];
        let observer = this.onChildComplete.bind(this);
        this.m_pBehaviorTree.start(this.m_Current, observer);
    }
    onChildComplete() {
        let child = this.m_Current;
        if (child.m_eStatus == Status_1.Status.BH_FAILURE) {
            this.m_pBehaviorTree.stop(this, Status_1.Status.BH_FAILURE);
            return;
        }
        Test_1.ASSERT(child.m_eStatus == Status_1.Status.BH_SUCCESS);
        this.m_Current = this.m_Children[++this.m_CurrentIndex];
        if (this.m_Current == null) {
            this.m_pBehaviorTree.stop(this, Status_1.Status.BH_SUCCESS);
        }
        else {
            let observer = this.onChildComplete.bind(this);
            this.m_pBehaviorTree.start(this.m_Current, observer);
        }
    }
    update() {
        return Status_1.Status.BH_RUNNING;
    }
}
exports.Sequence = Sequence;
//# sourceMappingURL=Sequence.js.map