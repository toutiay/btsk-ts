import { Behavior } from "./Behavior";
import { Decorator } from "./Decorator";
import { Status } from "../Enum/Status";

export class Repeat extends Decorator {
    m_iLimit: number;
    m_iCounter: number;

    constructor(child: Behavior) {
        super(child);
        this.m_iLimit = 0;
        this.m_iCounter = 0;
    }

    setCount(count: number) {
        this.m_iLimit = count;
    }

    onInitialize() {
        this.m_iCounter = 0;
    }

    update() {
        while (1) {
            this.m_pChild.tick();
            if (this.m_pChild.getStatus() == Status.BH_RUNNING) break;
            if (this.m_pChild.getStatus() == Status.BH_FAILURE) return Status.BH_FAILURE;
            if (++this.m_iCounter == this.m_iLimit) return Status.BH_SUCCESS;
            this.m_pChild.reset();
        }
        return Status.BH_INVALID;
    }
}