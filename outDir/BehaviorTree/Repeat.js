"use strict";
Object.defineProperty(exports, "__esModule", { value: true });
const Decorator_1 = require("./Decorator");
const Enum_1 = require("../Enum");
class Repeat extends Decorator_1.Decorator {
    constructor(child) {
        super(child);
        this.m_iLimit = 0;
        this.m_iCounter = 0;
    }
    setCount(count) {
        this.m_iLimit = count;
    }
    onInitialize() {
        this.m_iCounter = 0;
    }
    update() {
        while (1) {
            this.m_pChild.tick();
            if (this.m_pChild.getStatus() == Enum_1.Status.BH_RUNNING)
                break;
            if (this.m_pChild.getStatus() == Enum_1.Status.BH_FAILURE)
                return Enum_1.Status.BH_FAILURE;
            if (++this.m_iCounter == this.m_iLimit)
                return Enum_1.Status.BH_SUCCESS;
            this.m_pChild.reset();
        }
        return Enum_1.Status.BH_INVALID;
    }
}
exports.Repeat = Repeat;
//# sourceMappingURL=Repeat.js.map