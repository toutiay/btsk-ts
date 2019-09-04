import { Behavior } from "./Behavior";
import { Composite } from "./Composite";
import { Status } from "../Enum/Status";
import { Policy } from "../Enum/Policy";

export class Parallel extends Composite {
    m_eSuccessPolicy: number;
    m_eFailurePolicy: number;

    constructor(forSuccess: number, forFailure: number) {
        super();
        this.m_eSuccessPolicy = forSuccess;
        this.m_eFailurePolicy = forFailure;
    }

    update(): number {
        let iSuccessCount = 0, iFailureCount = 0;

        for (let i = 0; i < this.m_Children.length; i++) {
            let b: Behavior = this.m_Children[i];

            if (!b.isTerminated()) {
                b.tick();
            }
            if (b.getStatus() == Status.BH_SUCCESS) {
                ++iSuccessCount;
                if (this.m_eSuccessPolicy == Policy.RequireOne) {
                    return Status.BH_SUCCESS;
                }
            }

            if (b.getStatus() == Status.BH_FAILURE) {
                ++iFailureCount;
                if (this.m_eFailurePolicy == Policy.RequireOne) {
                    return Status.BH_FAILURE;
                }
            }
        }

        if (this.m_eFailurePolicy == Policy.RequireAll && iFailureCount == this.m_Children.length) {
            return Status.BH_FAILURE;
        }

        if (this.m_eSuccessPolicy == Policy.RequireAll && iSuccessCount == this.m_Children.length) {
            return Status.BH_SUCCESS;
        }

        return Status.BH_RUNNING;
    }

    onTerminate() {
        for (let i = 0; i < this.m_Children.length; i++) {
            let b: Behavior = this.m_Children[i];
            if (b.isRunning()) {
                b.abort();
            }
        }
    }
}