"use strict";
Object.defineProperty(exports, "__esModule", { value: true });
const Composite_1 = require("./Composite");
const Enum_1 = require("../Enum");
class Parallel extends Composite_1.Composite {
    constructor(forSuccess, forFailure) {
        super();
        this.m_eSuccessPolicy = forSuccess;
        this.m_eFailurePolicy = forFailure;
    }
    update() {
        let iSuccessCount = 0, iFailureCount = 0;
        for (let i = 0; i < this.m_Children.length; i++) {
            let b = this.m_Children[i];
            if (!b.isTerminated()) {
                b.tick();
            }
            if (b.getStatus() == Enum_1.Status.BH_SUCCESS) {
                ++iSuccessCount;
                if (this.m_eSuccessPolicy == Enum_1.Policy.RequireOne) {
                    return Enum_1.Status.BH_SUCCESS;
                }
            }
            if (b.getStatus() == Enum_1.Status.BH_FAILURE) {
                ++iFailureCount;
                if (this.m_eFailurePolicy == Enum_1.Policy.RequireOne) {
                    return Enum_1.Status.BH_FAILURE;
                }
            }
        }
        if (this.m_eFailurePolicy == Enum_1.Policy.RequireAll && iFailureCount == this.m_Children.length) {
            return Enum_1.Status.BH_FAILURE;
        }
        if (this.m_eSuccessPolicy == Enum_1.Policy.RequireAll && iSuccessCount == this.m_Children.length) {
            return Enum_1.Status.BH_SUCCESS;
        }
        return Enum_1.Status.BH_RUNNING;
    }
    onTerminate() {
        for (let i = 0; i < this.m_Children.length; i++) {
            let b = this.m_Children[i];
            if (b.isRunning()) {
                b.abort();
            }
        }
    }
}
exports.Parallel = Parallel;
//# sourceMappingURL=Parallel.js.map