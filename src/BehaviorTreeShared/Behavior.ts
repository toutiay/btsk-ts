import { ASSERT } from '../BehaviorTree/Test';
import { Task } from './Task';
import { Node } from './Node';
import { Default, Status } from '../Enum';

export class Behavior {
    m_pTask: Task;

    m_pNode: Node;

    m_eStatus: number;

    constructor(node: Node) {
        this.m_pTask = Default.UNDEFINED;
        this.m_pNode = Default.UNDEFINED;
        this.m_eStatus = Status.BH_INVALID;
        node && this.setup(node);
    }

    setup(node: Node) {
        this.teardown();

        this.m_pNode = node;
        this.m_pTask = node.create();
    }
    teardown() {
        if (this.m_pTask == null) {
            return;
        }

        ASSERT(this.m_eStatus != Status.BH_RUNNING);

        this.m_pNode.destroy(this.m_pTask);

        this.m_pTask = Default.UNDEFINED;
    }

    tick(): number {
        if (this.m_eStatus == Status.BH_INVALID) {
            this.m_pTask.onInitialize();
        }

        this.m_eStatus = this.m_pTask.update();

        if (this.m_eStatus != Status.BH_RUNNING) {
            this.m_pTask.onTerminate(this.m_eStatus);
        }

        return this.m_eStatus;
    }

    get<T extends Task>(): T {
        return this.m_pTask as T;
    }
}