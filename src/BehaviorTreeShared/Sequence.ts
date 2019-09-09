import { Task } from "./Task";
import { Composite } from "./Composite";
import { Behavior } from "./Behavior";
import { Node } from './Node';
import { Status, Default } from "../Enum";

export class Sequence extends Task {
    m_CurrentIndex: number;
    m_CurrentChild: Node;
    m_CurrentBehavior: Behavior;

    constructor(node: Composite) {
        super(node);
        this.m_CurrentIndex = 0;
        this.m_CurrentChild = Default.UNDEFINED;
        this.m_CurrentBehavior = new Behavior(Default.UNDEFINED);
    }

    getNode(): Composite {
        return this.m_pNode as Composite;
    }

    onInitialize() {
        this.m_CurrentIndex = 0;
        this.m_CurrentChild = this.getNode().m_Children[this.m_CurrentIndex];
        this.m_CurrentBehavior.setup(this.m_CurrentChild);
    }

    update() {
        while (true) {
            let s = this.m_CurrentBehavior.tick();
            if (s != Status.BH_SUCCESS) {
                return s;
            }
            this.m_CurrentIndex++;
            if (this.m_CurrentIndex == this.getNode().m_Children.length) {
                return Status.BH_SUCCESS;
            }
            this.m_CurrentChild = this.getNode().m_Children[this.m_CurrentIndex];
            this.m_CurrentBehavior.setup(this.m_CurrentChild);
        }
    }
}