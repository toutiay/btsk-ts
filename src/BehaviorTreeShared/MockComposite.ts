import { Composite as Composite } from '../BehaviorTreeShared/Composite';
import { MockTask } from "../BehaviorTreeShared/MockTask";
import { MockNode } from "../BehaviorTreeShared/MockNode";
import { Task } from '../BehaviorTreeShared/Task';
import { ASSERT } from "../BehaviorTree/Test";

export function createClass(fname: String, TASK: { new(node: Composite): Task; }) {
    let c = class extends Composite {
        constructor(size: number) {
            super();
            for (let i = 0; i < size; i++) {
                this.m_Children.push(new MockNode);
            }
        }

        getOperator(index: number): MockTask {
            ASSERT(index < this.m_Children.length);
            let task = (this.m_Children[index] as MockNode).m_pTask;
            ASSERT(task != null);
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