import { Status } from './../Enum/Status';
import { Task } from './Task';
import { Node } from './Node';

export class MockTask extends Task {
    m_iInitializeCalled: number;
    m_iTerminateCalled: number;
    m_iUpdateCalled: number;
    m_eReturnStatus: number;
    m_eTerminateStatus: number;
    constructor(node: Node) {
        super(node);

        this.m_iInitializeCalled = 0;
        this.m_iTerminateCalled = 0;
        this.m_iUpdateCalled = 0;
        this.m_eReturnStatus = Status.BH_RUNNING;
        this.m_eTerminateStatus = Status.BH_INVALID;
    }

    onInitialize() {
        ++this.m_iInitializeCalled;
    }

    onTerminate(s: number) {
        ++this.m_iTerminateCalled;
        this.m_eTerminateStatus = s;
    }

    update(): number {
        ++this.m_iUpdateCalled;
        return this.m_eReturnStatus;
    }
}