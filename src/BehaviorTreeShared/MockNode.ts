import { Node } from "./Node";
import { MockTask } from './MockTask';
import { Default } from "../Enum";

export class MockNode extends Node {
    m_pTask: MockTask;
    constructor() {
        super();
        this.m_pTask = Default.UNDEFINED;
    }

    create(): MockTask {
        this.m_pTask = new MockTask(this);
        return this.m_pTask;
    }

    destroy() {

    }
}