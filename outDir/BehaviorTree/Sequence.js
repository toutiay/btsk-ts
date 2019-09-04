"use strict";
Object.defineProperty(exports, "__esModule", { value: true });
const Behavior_1 = require("./Behavior");
const Composite_1 = require("./Composite");
const Status_1 = require("../Enum/Status");
class Sequence extends Composite_1.Composite {
    constructor() {
        super();
        this.m_CurrentIndex = 0;
        this.m_CurrentChild = new Behavior_1.Behavior;
    }
    onInitialize() {
        this.m_CurrentIndex = 0;
        this.m_CurrentChild = this.m_Children[this.m_CurrentIndex];
    }
    update() {
        // Keep going until a child behavior says it's running.
        while (1) {
            let s = this.m_CurrentChild.tick();
            // If the child fails, or keeps running, do the same.
            if (s != Status_1.Status.BH_SUCCESS) {
                return s;
            }
            // Hit the end of the array, job done!
            this.m_CurrentIndex++;
            if (this.m_CurrentIndex == this.m_Children.length) {
                return Status_1.Status.BH_SUCCESS;
            }
            this.m_CurrentChild = this.m_Children[this.m_CurrentIndex];
        }
        return Status_1.Status.BH_FAILURE;
    }
}
exports.Sequence = Sequence;
//# sourceMappingURL=Sequence.js.map