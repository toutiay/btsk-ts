import { Behavior } from "./Behavior";
import { Default, Status } from "../Enum";
import { ASSERT } from "../Utils";

export class BehaviorTree {
    m_Behaviors: Behavior[] = [];

    start(bh: Behavior, observer: Function = Default.UNDEFINED) {
        if (observer != null) {
            bh.m_Observer = observer;
        }
        this.m_Behaviors.unshift(bh);
    }

    stop(bh: Behavior, result: number) {
        ASSERT(result != Status.BH_RUNNING);
        bh.m_eStatus = result;

        if (bh.m_Observer) {
            bh.m_Observer(result);
        }
    }

    tick() {
        // Insert an end-of-update marker into the list of tasks.
        this.m_Behaviors.push(Default.UNDEFINED);

        // Keep going updating tasks until we encounter the marker.
        while (this.step()) {
            continue;
        }
    }

    step(): boolean {
        let current = this.m_Behaviors.shift();

        // If this is the end-of-update marker, stop processing.
        if (current == null) {
            return false;
        }

        // Perform the update on this individual task.
        current.tick();

        // Process the observer if the task terminated.
        if (current.m_eStatus != Status.BH_RUNNING && current.m_Observer) {
            current.m_Observer(current.m_eStatus);
        }
        else // Otherwise drop it into the queue for the next tick().
        {
            this.m_Behaviors.push(current);
        }
        return true;
    }
}