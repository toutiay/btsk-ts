import { Behavior } from "./Behavior";
import { Composite } from "./Composite";
import { Status } from "../Enum/Status";

export class Sequence extends Composite {
    m_CurrentIndex: number;
    m_CurrentChild: Behavior;

    constructor() {
        super();
        this.m_CurrentIndex = 0;
        this.m_CurrentChild = new Behavior;
    }

    onInitialize() {
        this.m_CurrentIndex = 0;
        this.m_CurrentChild = this.m_Children[this.m_CurrentIndex];
    }

    update(): number {
        // Keep going until a child behavior says it's running.
        while (1) {
            let s: number = this.m_CurrentChild.tick();

            // If the child fails, or keeps running, do the same.
            if (s != Status.BH_SUCCESS) {
                return s;
            }

            // Hit the end of the array, job done!
            this.m_CurrentIndex++;
            if (this.m_CurrentIndex == this.m_Children.length) {
                return Status.BH_SUCCESS;
            }
            this.m_CurrentChild = this.m_Children[this.m_CurrentIndex];
        }
        return Status.BH_FAILURE;
    }
}