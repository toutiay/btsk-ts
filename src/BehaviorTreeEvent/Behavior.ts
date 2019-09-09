import { Status, Default } from "../Enum";


export class Behavior {
    m_eStatus: number;
    m_Observer: Function;
    constructor() {
        this.m_eStatus = Status.BH_INVALID;
        this.m_Observer = Default.UNDEFINED;
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

}