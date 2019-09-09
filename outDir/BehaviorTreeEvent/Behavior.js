"use strict";
Object.defineProperty(exports, "__esModule", { value: true });
const Status_1 = require("../Enum/Status");
const Default_1 = require("../Enum/Default");
class Behavior {
    constructor() {
        this.m_eStatus = Status_1.Status.BH_INVALID;
        this.m_Observer = Default_1.Default.UNDEFINED;
    }
    tick() {
        if (this.m_eStatus != Status_1.Status.BH_RUNNING) {
            this.onInitialize();
        }
        this.m_eStatus = this.update();
        if (this.m_eStatus != Status_1.Status.BH_RUNNING) {
            this.onTerminate(this.m_eStatus);
        }
        return this.m_eStatus;
    }
    onTerminate(m_eStatus) {
    }
    onInitialize() {
    }
    update() {
        return 0;
    }
}
exports.Behavior = Behavior;
//# sourceMappingURL=Behavior.js.map