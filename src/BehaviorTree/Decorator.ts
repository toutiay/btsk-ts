import { Behavior } from "./Behavior";

export class Decorator extends Behavior {
    m_pChild: Behavior;
    constructor(child: Behavior) {
        super();
        this.m_pChild = child;
    }
}