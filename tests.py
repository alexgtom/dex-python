import unittest
from rex import Rex

TEST_STRING = """
E-FILE HEADER-->  Record size: 8
100113_SO1
Tue Oct  1 15:08:48 2013

NEI/LSR REX 7.1

--------------------------------------
A-FILE HEADER-->  Record size: 20
100113_SO1
Tue Oct  1 15:08:48 2013

NEI/LSR REX 7.1

--------------------------------------

    Code    Trial Time	Relative Time

Trial 1:  3 events
      1001	0	0
      -112	419	-43674	1
         2	1250	1250

Trial 2:  2 events
      -112	419	-43674	1
         1	369	4114

"""

class RexTest(unittest.TestCase):
    def assertEventEqual(self, event, code, trial_time, relative_time, is_continue_record):
        self.assertEqual(event.code, code)
        self.assertEqual(event.trial_time, trial_time)
        self.assertEqual(event.relative_time, relative_time)
        self.assertEqual(event.is_continue_record, is_continue_record)

    def test_rex(self):
        trials_list = Rex._parse(TEST_STRING)
        trial1 = trials_list[0]
        trial2 = trials_list[1]
        
        self.assertEventEqual(trial1.events[0], 1001, 0, 0, False)
        self.assertEventEqual(trial1.events[1], -112, 419, -43674, True)
        self.assertEventEqual(trial1.events[2], 2, 1250, 1250, False)
        self.assertEqual(len(trial1.events), 3)
        self.assertEqual(trial1.trial_num, 1)

        self.assertEventEqual(trial2.events[0], -112, 419, -43674, True)
        self.assertEventEqual(trial2.events[1], 1, 369, 4114, False)
        self.assertEqual(len(trial2.events), 2)
        self.assertEqual(trial2.trial_num, 2)

    def test_rex_blank(self):
        trials_list = Rex._parse("")
        self.assertEqual(len(trials_list), 0)

    def test_str(self):
        trials_list = Rex._parse(TEST_STRING)
        self.assertEqual(repr(trials_list), 
            "[<Trial 1: {events: [<Event {code: 1001, trial_time: 0, relative_time: 0, "
            "is_continue_record False}>, <Event {code: -112, trial_time: "
            "419, relative_time: -43674, is_continue_record True}>, <Event "
            "{code: 2, trial_time: 1250, relative_time: 1250, "
            "is_continue_record False}>]}, <Trial 2: {events: [<Event "
            "{code: -112, trial_time: 419, relative_time: -43674, "
            "is_continue_record True}>, <Event {code: 1, trial_time: 369, "
            "relative_time: 4114, is_continue_record False}>]}]")

if __name__ == "__main__":
    unittest.main()
