import { Task } from './../BehaviorTreeShared/Task';
import { MockBehavior } from "./MockBehavior";
import { Composite } from "../BehaviorTree/Composite";
import { Composite as CompositeShared } from './../BehaviorTreeShared/Composite';
import { MockNode } from "../BehaviorTreeShared/MockNode";
import Test from "./Test";
import { MockTask } from "../BehaviorTreeShared/MockTask";

export function createClass(fname: String, COMPOSITE: { new(): Composite; }) {
    let c = class extends COMPOSITE {
        constructor(size: number) {
            super();
            for (let i = 0; i < size; i++) {
                this.m_Children.push(new MockBehavior);
            }
        }

        getOperator(index: number): MockBehavior {
            return this.m_Children[index] as MockBehavior;
        }
    }
    return c;
}

export function createClass1(fname: String, TASK: { new(node: CompositeShared): Task; }) {
    let c = class extends CompositeShared {
        constructor(size: number) {
            super();
            for (let i = 0; i < size; i++) {
                this.m_Children.push(new MockNode);
            }
        }

        getOperator(index: number): MockTask {
            Test.ASSERT(index < this.m_Children.length);
            let task = (this.m_Children[index] as MockNode).m_pTask;
            Test.ASSERT(task != null);
            return task;
        }

        create(): Task {
            return new TASK(this);
        }

        destroy(t: Task) {

        }
    }
    return c;
}