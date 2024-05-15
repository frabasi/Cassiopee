# Compare session logs printed by validCassiopee
# Usage: python compareSessionLogs.py --logs='log1 log2'
#        python compareSessionLogs.py --logs='log1 log2' --email
# Differences written in: compSession_DATE.txt where DATE is the current time
import os
from time import localtime, strftime

# Parse command-line arguments
def parseArgs():
    import argparse
    # Create argument parser
    parser = argparse.ArgumentParser()
    parser.add_argument("-e", "--email", action="store_true",
                        help="Email results. Default: write to file")
    parser.add_argument("-l", "--logs", type=str, default='',
                        help="Single-quoted logs to compare.")
    # Parse arguments
    return parser.parse_args()

# Read git info in a session log of validCassiopee
def readGitInfo(filename):
  if not os.access(filename, os.R_OK):
    raise Exception("Session log can't be read: {}".format(filename))
    
  gitInfo = dict.fromkeys(['Base', 'Git origin', 'Git branch', 'Commit hash'])
  with open (filename, 'r') as f:
    for line in f:
      if 'Base from' in line:
        gitInfo['Base'] = line.strip().split(' ')[-1]
      if 'Git origin' in line:
        gitInfo['Git origin'] = line.strip().split(' ')[-1]
      if 'Git branch' in line:
        info = line.strip().split(' ')
        gitInfo['Git branch'] = info[2][:-1]
        gitInfo['Commit hash'] = info[-1]
      if all(v is not None for v in gitInfo.values()):  
        break
  return gitInfo
  
# Read a session log of validCassiopee
def readLog(filename):
  if not os.access(filename, os.R_OK):
    raise Exception("Session log can't be read: {}".format(filename))
    
  session = []
  with open (filename, 'r') as f:
    for line in f:
      if ' :' in line: 
        test = line.strip().split(':')
        session.append([e.strip() for e in test])
  return session

# Check the status of two tests using the test strings of validCassiopee
def diffTest(test, ref, new):
  refStatus = ref[5]
  newStatus = new[5]
  if refStatus != newStatus:
    return test, ref, new
  else:
    return ''

# Stringify test comparison
def stringify(test='', ref='', new=''):
  if not test:
    return test
  mod, test = test.split('/')
  test.split('.')[0]
  if not (ref or new):
    return "{:>15} | {:>42} |\n".format(mod, test)
  else:
    return "{:>15} | {:>42} | {:>10} | {:>10} |\n".format(mod, test, ref[5], new[5])

# Send an email
def notify(sender=None, recipients=[], messageSubject="", messageText=""):
    try:
      import smtplib
      from email.mime.text import MIMEText
      from email.mime.multipart import MIMEMultipart

      if sender is None:
        if os.getenv('CASSIOPEE_EMAIL') is None:
            if os.getenv('USER') is None:
              print("Sender email address not found.")
              return
            else: sender = os.getenv('USER')+'@onera.fr'
        else: sender = os.getenv('CASSIOPEE_EMAIL')
      if isinstance(recipients, str): recipients = [recipients]
      if not recipients: recipients = ['vincent.casseau@onera.fr',
                                       'christophe.benoit@onera.fr']
      
      msg = MIMEMultipart()
      msg['Subject'] = messageSubject
      msg['From'] = sender
      msg['To'] = ", ".join(recipients)
      msg['Cc'] = sender
      msg.preamble = 'Sent by Cassiopee.'
      if messageText:
        msg.attach(MIMEText(messageText, 'plain'))
      s = smtplib.SMTP('localhost')
      s.sendmail(sender, recipients, msg.as_string())
      s.quit()
    except: return

# Main
if __name__ == '__main__':
  script_args = parseArgs()
  script_args.logs = script_args.logs.split(' ')
  if len(script_args.logs) != 2:
    raise Exception("Two session logs must be provided using the flag -l "
                    "or --logs")
  
  # Read log files and git info
  refSession = readLog(script_args.logs[0])
  newSession = readLog(script_args.logs[1])
  gitInfo = readGitInfo(script_args.logs[1])
  
  # Draw a one-to-one correspondance between tests of each session
  # (module + testname)
  refDict = dict((test[0] + '/' + test[1], test[2:]) for test in refSession)
  newDict = dict((test[0] + '/' + test[1], test[2:]) for test in newSession)
  refSet = set(refDict)
  newSet = set(newDict)
  
  # Find common tests
  commonTests = sorted(refSet & newSet)
  # Find new tests (tests in newSession but not in refSession)
  newTests = sorted(newSet - refSet)
  # Find deleted tests (tests in refSession but not in newSession)
  deletedTests = sorted(refSet - newSet)
  
  # Write differences to file
  baseState = 'OK'
  compStr = ""
  header = "\n".join("{}: {}".format(k,v) for k,v in gitInfo.items())
  header += "\n\nREF = {}\n".format(script_args.logs[0])
  header += "NEW = {}\n\n\n".format(script_args.logs[1])
  header += "{} | {} | {} |\n{}\n".format("TESTS".center(60), "REF".center(10),
                                          "NEW".center(10), '*'*88)
  commonTestsHeader = "Tests that differ:\n{}\n".format('-'*17)
  for test in commonTests:
    compStr += stringify(*diffTest(test, refDict[test], newDict[test]))
  if len(compStr):
    compStr = commonTestsHeader + compStr
    baseState = 'FAIL'
  else: compStr = commonTestsHeader + "[none]\n"
  
  newTestsHeader = "\nNew tests:\n{}\n".format('-'*9)
  if len(newTests):
    compStr += newTestsHeader
    for test in newTests:
      compStr += stringify(test)
    if baseState == 'OK': baseState = 'NEW ADDITIONS'
  else: compStr += newTestsHeader + "[none]\n"
  
  deletedTestsHeader = "\nDeleted tests:\n{}\n".format('-'*13)
  if len(deletedTests):
    compStr += deletedTestsHeader
    for test in deletedTests:
      compStr += stringify(test)
    baseState = 'FAIL'
  else: compStr += deletedTestsHeader + "[none]\n"
    
  locTime = localtime()
  now = strftime("%d/%m/%y at %T", locTime)
  now2 = strftime("%y%m%d-%H%M%S", locTime)
  if script_args.email:
    notify(messageSubject="[validCassiopee] {} - "
             "State: {}".format(now, baseState),
             messageText=header + compStr)
  else:
    filename = "./compSession_{}.txt".format(now2)
    if os.access('./', os.W_OK):
      print("Writing comparison to {}".format(filename))
      with open(filename, 'w') as f: f.write(header + compStr)
