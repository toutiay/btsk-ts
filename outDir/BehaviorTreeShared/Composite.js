"use strict";
Object.defineProperty(exports, "__esModule", { value: true });
const Node_1 = require("./Node");
class Composite extends Node_1.Node {
    constructor() {
        super(...arguments);
        this.m_Children = [];
    }
}
exports.Composite = Composite;
//# sourceMappingURL=Composite.js.map