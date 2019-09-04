"use strict";
Object.defineProperty(exports, "__esModule", { value: true });
const Policy_1 = require("../Enum/Policy");
const Parallel_1 = require("./Parallel");
class Monitor extends Parallel_1.Parallel {
    constructor() {
        super(Policy_1.Policy.RequireOne, Policy_1.Policy.RequireOne);
    }
    addCondition(condition) {
        this.m_Children.unshift(condition);
    }
    addAction(action) {
        this.m_Children.push(action);
    }
}
exports.Monitor = Monitor;
//# sourceMappingURL=Monitor.js.map