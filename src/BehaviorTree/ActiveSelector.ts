import { Status } from "./Status";
import { Behavior } from "./Behavior";
import { Selector } from "./Selector";

export class ActiveSelector extends Selector {
    onInitialize() {
        this.m_Current = this.m_Children[this.m_Children.length];
    }

    update() {
        let previous: Behavior = this.m_Current;

        super.onInitialize();

        let result: number = super.update();

        if (previous != this.m_Children[this.m_Children.length] && this.m_Current != previous) {
            previous.onTerminate(Status.BH_ABORTED);
        }
        return result;
    }
}