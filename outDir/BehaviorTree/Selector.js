"use strict";
Object.defineProperty(exports, "__esModule", { value: true });
const Behavior_1 = require("./Behavior");
const Composite_1 = require("./Composite");
const Status_1 = require("../Enum/Status");
class Selector extends Composite_1.Composite {
    constructor() {
        super();
        this.m_CurrentIndex = 0;
        this.m_Current = new Behavior_1.Behavior;
    }
    onInitialize() {
        this.m_CurrentIndex = 0;
        this.m_Current = this.m_Children[this.m_CurrentIndex];
    }
    update() {
        // Keep going until a child behavior says its running.
        while (1) {
            let s = this.m_Current.tick();
            // If the child succeeds, or keeps running, do the same.
            if (s != Status_1.Status.BH_FAILURE) {
                return s;
            }
            this.m_CurrentIndex++;
            // Hit the end of the array, it didn't end well...
            if (this.m_CurrentIndex == this.m_Children.length) {
                return Status_1.Status.BH_FAILURE;
            }
            this.m_Current = this.m_Children[this.m_CurrentIndex];
        }
        return Status_1.Status.BH_FAILURE;
    }
}
exports.Selector = Selector;
//# sourceMappingURL=Selector.js.map