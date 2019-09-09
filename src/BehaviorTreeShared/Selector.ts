import { Task } from './Task';
import { Composite } from './Composite';
import { Behavior } from './Behavior';
import { Node } from './Node';
import { Default, Status } from '../Enum';

export class Selector extends Task {
    m_CurrentIndex: number;
    m_CurrentChild: Node;
    m_Behavior: Behavior;

    constructor(node: Composite) {
        super(node);
        this.m_CurrentIndex = 0;
        this.m_CurrentChild = Default.UNDEFINED;
        this.m_Behavior = new Behavior(Default.UNDEFINED);
    }

    getNode(): Composite {
        return this.m_pNode as Composite;
    }

    onInitialize() {
        this.m_CurrentIndex = 0;
        this.m_CurrentChild = this.getNode().m_Children[this.m_CurrentIndex];
        this.m_Behavior.setup(this.m_CurrentChild);
    }

    update() {
        while (true) {
            let s = this.m_Behavior.tick();
            if (s != Status.BH_FAILURE) {
                return s;
            }
            this.m_CurrentIndex++;
            if (this.m_CurrentIndex == this.getNode().m_Children.length) {
                return Status.BH_FAILURE;
            }
            this.m_CurrentChild = this.getNode().m_Children[this.m_CurrentIndex];
            this.m_Behavior.setup(this.m_CurrentChild);
        }
    }
}