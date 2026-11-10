"""Check grouped-platform ownership and number limits without ESPHome imports."""
import ast
from pathlib import Path
from types import SimpleNamespace
import unittest

ROOT = Path(__file__).resolve().parents[1] / "components" / "hi_link_ld1125h"


def load_nodes(path, names, scope):
    tree = ast.parse(path.read_text())
    nodes = [node for node in tree.body
             if isinstance(node, ast.FunctionDef) and node.name in names
             or isinstance(node, ast.Assign) and any(isinstance(target, ast.Name) and target.id in names for target in node.targets)]
    exec(compile(ast.Module(body=nodes, type_ignores=[]), str(path), "exec"), scope)


class SchemaValidationTest(unittest.TestCase):
    def test_group_ownership(self):
        full_config = {}
        scope = {"cv": SimpleNamespace(Invalid=ValueError),
                 "fv": SimpleNamespace(full_config=SimpleNamespace(get=lambda: full_config)),
                 "CONF_HUB_ID": "hi_link_ld1125h_id"}
        load_nodes(ROOT / "__init__.py", {"validate_platform"}, scope)
        check = scope["validate_platform"]("sensor")
        own = {"platform": "hi_link_ld1125h", "hi_link_ld1125h_id": "radar"}
        other = {**own, "hi_link_ld1125h_id": "other_radar"}
        full_config["sensor"] = [own, other, {"platform": "template"}]
        self.assertIs(check(own), own)
        full_config["sensor"].append(own)
        with self.assertRaises(ValueError):
            check(own)
        # Another domain can expose a different group for the same radar.
        full_config["sensor"] = [own]
        full_config["number"] = [own]
        self.assertIs(check(own), own)

    def test_number_limits(self):
        scope = {"cv": SimpleNamespace(Invalid=ValueError)}
        load_nodes(ROOT / "number.py", {"PARAMETERS", "validate_limits"}, scope)
        check = scope["validate_limits"]
        self.assertEqual(check({}), {})
        for key in scope["PARAMETERS"]:
            with self.subTest(key=key):
                valid = {key: {"min_value": 1, "max_value": 2}}
                self.assertIs(check(valid), valid)
                # Bounds must match the parameter's on-wire resolution.
                step = scope["PARAMETERS"][key][3]
                for minimum, maximum in ((1, 1), (2, 1), (1 + step / 2, 2), (1, 2 + step / 2)):
                    with self.assertRaises(ValueError):
                        check({key: {"min_value": minimum, "max_value": maximum}})


if __name__ == "__main__":
    unittest.main()
