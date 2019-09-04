import { Status } from "../Enum/Status";

export class Behavior {
    m_eStatus: number;
    constructor() {
        this.m_eStatus = Status.BH_INVALID;
    }

    tick(): number {
        if (this.m_eStatus != Status.BH_RUNNING) {
            this.onInitialize();
        }

        this.m_eStatus = this.update();

        if (this.m_eStatus != Status.BH_RUNNING) {
            this.onTerminate(this.m_eStatus);
        }
        return this.m_eStatus;
    }

    onTerminate(m_eStatus: number) {

    }

    onInitialize() {

    }

    update(): number {
        return 0;
    }

    reset() {
        this.m_eStatus = Status.BH_INVALID;
    }

    abort() {
        this.onTerminate(Status.BH_ABORTED);
        this.m_eStatus = Status.BH_ABORTED;
    }

    isTerminated(): boolean {
        return this.m_eStatus == Status.BH_SUCCESS || this.m_eStatus == Status.BH_FAILURE;
    }

    isRunning(): boolean {
        return this.m_eStatus == Status.BH_RUNNING;
    }

    getStatus(): number {
        return this.m_eStatus;
    }
}