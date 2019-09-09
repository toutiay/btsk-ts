"use strict";
Object.defineProperty(exports, "__esModule", { value: true });
var Status;
(function (Status) {
    Status[Status["BH_INVALID"] = 0] = "BH_INVALID";
    Status[Status["BH_SUCCESS"] = 1] = "BH_SUCCESS";
    Status[Status["BH_FAILURE"] = 2] = "BH_FAILURE";
    Status[Status["BH_RUNNING"] = 3] = "BH_RUNNING";
    Status[Status["BH_ABORTED"] = 4] = "BH_ABORTED";
    Status[Status["BH_SUSPENDED"] = 5] = "BH_SUSPENDED";
})(Status = exports.Status || (exports.Status = {}));
var Policy;
(function (Policy) {
    Policy[Policy["RequireOne"] = 0] = "RequireOne";
    Policy[Policy["RequireAll"] = 1] = "RequireAll";
})(Policy = exports.Policy || (exports.Policy = {}));
class Default {
}
Default.UNDEFINED = [][0];
Default.NULL = [][0];
exports.Default = Default;
//# sourceMappingURL=Enum.js.map