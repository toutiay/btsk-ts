import { Behavior } from "./Behavior";
import { Composite } from "./Composite";
import { Status } from "../Enum";

export class Selector extends Composite {
    m_CurrentIndex: number;
    m_Current: Behavior;

    constructor() {
        super();
        this.m_CurrentIndex = 0;
        this.m_Current = new Behavior;
    }

    onInitialize() {
        this.m_CurrentIndex = 0;
        this.m_Current = this.m_Children[this.m_CurrentIndex];
    }

    update(): number {
        // Keep going until a child behavior says its running.
        while (1) {
            let s: number = this.m_Current.tick();

            // If the child succeeds, or keeps running, do the same.
            if (s != Status.BH_FAILURE) {
                return s;
            }

            this.m_CurrentIndex++;
            // Hit the end of the array, it didn't end well...
            if (this.m_CurrentIndex == this.m_Children.length) {
                return Status.BH_FAILURE;
            }
            this.m_Current = this.m_Children[this.m_CurrentIndex];
        }
        return Status.BH_FAILURE;
    }
}