# Send a notification by email to a list of recipients about the installation
# status of different prods. Please set the environment variable CASSIOPEE_EMAIL
# Usage: python notifyInstall.py --recipients='a.b@onera.fr c.d@onera.fr'
import sys
from time import strptime, strftime

# Parse command-line arguments
def parseArgs():
  import argparse
  # Create argument parser
  parser = argparse.ArgumentParser()
  parser.add_argument("-r", "--recipients", type=str, default='',
                      help="Single-quoted space-separated list of recipients")
  # Parse arguments
  return parser.parse_args()
    
# Main
if __name__ == '__main__':
  try:
    import KCore.Dist as Dist
    from KCore.notify import notify
  except ImportError:
    print("Error: KCore is required to execute notifyInstall.py")
    sys.exit()
  
  script_args = parseArgs()
  recipients = script_args.recipients.split(' ')
  if not recipients[0]: recipients = []
  
  # Check install status
  log_entries = []
  with open('/stck/cassiope/git/logs/installation_status.txt', 'r') as f:
    for line in f:
      log_entries.append(line.strip().split(' - '))
  log_entries.sort(key=lambda x: x[1], reverse=True)
  
  # Get git info
  cassiopeeIncDir = '/stck/cassiope/git/Cassiopee/Cassiopee'
  gitOrigin = Dist.getGitOrigin(cassiopeeIncDir)
  gitBranch = Dist.getGitBranch(cassiopeeIncDir)
  gitHash = Dist.getGitHash(cassiopeeIncDir)[:7]
  gitInfo = "Git origin: {}\nGit branch: {}\nCommit hash: {}".format(
    gitOrigin, gitBranch, gitHash)
      
  baseState = 'OK'
  messageText = "Installation of Cassiopee, Fast and all "\
    "PModules:\n{}\n\n{}\n\n\n".format(48*'-', gitInfo)
  for log_machine in log_entries:
    prod = log_machine[0]
    date = strptime(log_machine[1], "%y%m%d-%H%M%S")
    date = strftime("%d/%m/%y at %T", date)
    status = log_machine[2]
    messageText += '{:>20} |      {}      | {:>10}\n'.format(prod, date, status)
    if 'FAILED' in log_machine: baseState = 'FAILED'
    
  if baseState == 'FAILED':
    messageText += '\n\nIf the prod. you wish to use is marked as FAILED, '\
      'please contact the maintainers:\nchristophe.benoit@onera.fr, '\
      'vincent.casseau@onera.fr'
  
  notify(recipients=recipients,
         messageSubject="[Install Cassiopee] State: {}".format(baseState),
         messageText=messageText)
