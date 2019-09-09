import { MockBehavior } from "./MockBehavior";
import { Composite } from "./Composite";

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