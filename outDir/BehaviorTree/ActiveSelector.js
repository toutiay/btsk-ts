"use strict";
Object.defineProperty(exports, "__esModule", { value: true });
const Status_1 = require("../Enum/Status");
const Selector_1 = require("./Selector");
class ActiveSelector extends Selector_1.Selector {
    onInitialize() {
        this.m_Current = this.m_Children[this.m_Children.length];
    }
    update() {
        let previous = this.m_Current;
        super.onInitialize();
        let result = super.update();
        if (previous != this.m_Children[this.m_Children.length] && this.m_Current != previous) {
            previous.onTerminate(Status_1.Status.BH_ABORTED);
        }
        return result;
    }
}
exports.ActiveSelector = ActiveSelector;
//# sourceMappingURL=ActiveSelector.js.map