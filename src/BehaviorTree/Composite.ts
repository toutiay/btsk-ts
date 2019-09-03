import { Behavior } from "./Behavior";

export class Composite extends Behavior {
    m_Children: Array<Behavior> = [];

    addChild(child: Behavior) {
        this.m_Children.push(child);
    }

    removeChild(child: Behavior) {
        let index = this.m_Children.indexOf(child);
        if (index != -1) {
            this.m_Children.splice(index, 1);
        }
    }

    clearChildren() {
        this.m_Children.length = 0;
    }
}