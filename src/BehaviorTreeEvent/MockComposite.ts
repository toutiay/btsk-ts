import { Composite } from "./Composite";
import { BehaviorTree } from "./BehaviorTree";
import { MockBehavior } from "./MockBehavior";
import { ASSERT } from "../Utils";

export function createClass(fname: String, COMPOSITE: { new(bt: BehaviorTree): Composite; }) {
    let c = class extends COMPOSITE {
        constructor(bt: BehaviorTree, size: number) {
            super(bt);
            for (let i = 0; i < size; i++) {
                this.m_Children.push(new MockBehavior);
            }
        }

        getOperator(index: number): MockBehavior {
            ASSERT(index < this.m_Children.length);
            return (this.m_Children[index] as MockBehavior);
        }
    }
    return c;
}