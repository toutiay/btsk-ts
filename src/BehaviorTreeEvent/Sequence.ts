import { Composite } from "./Composite";
import { BehaviorTree } from "./BehaviorTree";
import { Behavior } from "./Behavior";
import { ASSERT } from "../BehaviorTree/Test";
import { Default, Status } from "../Enum";

export class Sequence extends Composite {
    m_pBehaviorTree: BehaviorTree;

    m_CurrentIndex: number;
    m_Current: Behavior;
    constructor(bt: BehaviorTree) {
        super();
        this.m_pBehaviorTree = bt;
        this.m_CurrentIndex = 0;
        this.m_Current = Default.UNDEFINED;
    }

    onInitialize() {
        this.m_CurrentIndex = 0;
        this.m_Current = this.m_Children[this.m_CurrentIndex];
        let observer: Function = this.onChildComplete.bind(this);
        this.m_pBehaviorTree.start(this.m_Current, observer);
    }

    onChildComplete() {
        let child = this.m_Current;
        if (child.m_eStatus == Status.BH_FAILURE) {
            this.m_pBehaviorTree.stop(this, Status.BH_FAILURE);
            return;
        }

        ASSERT(child.m_eStatus == Status.BH_SUCCESS);
        this.m_Current = this.m_Children[++this.m_CurrentIndex];
        if (this.m_Current == null) {
            this.m_pBehaviorTree.stop(this, Status.BH_SUCCESS);
        }
        else {
            let observer = this.onChildComplete.bind(this);
            this.m_pBehaviorTree.start(this.m_Current, observer);
        }
    }

    update(): number {
        return Status.BH_RUNNING;
    }
}