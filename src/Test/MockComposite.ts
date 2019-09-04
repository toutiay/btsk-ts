import { Selector } from "../BehaviorTree/Selector";
import { MockBehavior } from "./MockBehavior";
import { Sequence } from "../BehaviorTree/Sequence";
import { ActiveSelector } from "../BehaviorTree/ActiveSelector";
import { Composite } from "../BehaviorTree/Composite";

// export class MockSelector extends Selector {
//     constructor(size: number) {
//         super();
//         createSize(this, size);
//     }

//     getOperator(index: number): MockBehavior {
//         return getMock(this, index);
//     }
// }

// export class MockSequence extends Sequence {
//     constructor(size: number) {
//         super();
//         createSize(this, size);
//     }

//     getOperator(index: number): MockBehavior {
//         return getMock(this, index);
//     }
// }

// export class MockActiveSelector extends ActiveSelector {
//     constructor(size: number) {
//         super();
//         createSize(this, size);
//     }

//     getOperator(index: number): MockBehavior {
//         return getMock(this, index);
//     }
// }

// function create<T>(c: { new(): T; }): T {
//     return new c();
// }

export function createClass(fname: String, ftype: { new(): Composite; }) {
    let c: any = class extends ftype {
        constructor(size: number) {
            super();
            createSize(this, size);
        }

        getOperator(index: number): MockBehavior {
            return getMock(this, index);
        }
    }
    c.name = fname;
    return c;
}

function createSize(target: Composite, size: number) {
    for (let i = 0; i < size; i++) {
        target.m_Children.push(new MockBehavior);
    }
}

function getMock(target: Composite, index: number): MockBehavior {
    return target.m_Children[index] as MockBehavior;
}