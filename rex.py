"""
Wrapper for reading rex files
"""
import subprocess
import re

_TRDD_PATH = "./Dex/trdd/trdd"
_TRIAL_RE = re.compile('Trial (\d+):  (\d+) events')


def trdd(rex_file, start_code=1001, pre_time=0, max_trial_number=1000):
    """ runs trdd and returns the output from it """
    command = "{0} -s {1} -p {2} -M {3} {4}".format(
        _TRDD_PATH, 
        start_code, 
        pre_time,
        max_trial_number, 
        rex_file
    )
    return subprocess.check_output(command.split())


class Rex(object):
    """
    Creates a Rex object from a rex_file.

    A Rex object contains Trials. Trials contain Events.
    """
    def __init__(self, rex_file, **kwargs):
        """ open file using trdd and passing the arguements into kwargs """
        self.trials = self._parse(trdd(rex_file, **kwargs))

    def __repr__(self):
        return "<Rex {0}>".format(self.trials)

    @staticmethod
    def _parse(s):
        """ parse the output in s and create objects """
        trial_list = []
        lines = s.split('\n')
        # for each line in s
        i = 0
        while i < len(lines):
            # strip trailing and ending white space
            line = lines[i].strip() 

            # if we cannot find a trial header, go to next line
            match = re.search(_TRIAL_RE, line)
            i += 1

            if match == None:
                continue

            trial = Trial(int(match.group(1)))
            trial_list.append(trial)

            # create events for trial
            while i < len(lines):
                line = lines[i].strip()
                if len(line) == 0:
                    i += 1
                    break

                nums = [int(n) for n in line.split()]

                if len(nums) == 3:
                    is_continue_record = False
                elif len(nums) == 4:
                    if nums[3] == 0:
                        is_continue_record = False
                    else:
                        is_continue_record = True
                else:
                    raise SyntaxError(
                        "Expected 3 or 4 numbers per event but got {0} instead".format(len(nums)))

                trial.events.append(Event(nums[0], nums[1], nums[2], is_continue_record))

                i += 1

        return trial_list


class Trial(object):
    def __init__(self, trial_num):
        self.trial_num = trial_num
        self.events = []

    def __repr__(self):
        return "<Trial {0}: {{events: {1}}}".format(self.trial_num, str(self.events))


class Event(object):
    def __init__(self, code, trial_time, relative_time, is_continue_record):
        """
        is_continue_record -- true if is a continuation record, false otherwise
        """
        self.code = code
        self.trial_time = trial_time
        self.relative_time = relative_time
        self.is_continue_record = is_continue_record

    def __repr__(self):
        return "<Event {{code: {0}, trial_time: {1}, relative_time: {2}, is_continue_record {3}}}>".format(
            self.code, self.trial_time, self.relative_time, self.is_continue_record)
