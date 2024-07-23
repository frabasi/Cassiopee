# Send a notification by email to a list of recipients about the checkout
# status of different prods. Please set the environment variable CASSIOPEE_EMAIL
# Usage: python notifyCheckout.py --recipients='a.b@onera.fr c.d@onera.fr'
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
    print("Error: KCore is required to execute notifyCheckout.py")
    sys.exit()
  
  script_args = parseArgs()
  recipients = script_args.recipients.split(' ')
  if not recipients[0]: recipients = []
  
  # Check checkout status
  log_entries = []
  with open('/stck/cassiope/git/logs/checkout_status.txt', 'r') as f:
    for line in f:
      log_entries.append(line.strip().split(' - '))
  log_entries.sort(key=lambda x: x[1], reverse=True)
  
  # Do not send a notification when everything is OK
  if not any('FAILED' in log_machine for log_machine in log_entries): sys.exit()
  
  # Get git info
  cassiopeeIncDir = '/stck/cassiope/git/Cassiopee/Cassiopee'
  gitOrigin = Dist.getGitOrigin(cassiopeeIncDir)
  gitBranch = Dist.getGitBranch(cassiopeeIncDir)
  gitHash = Dist.getGitHash(cassiopeeIncDir)[:7]
  gitInfo = "Git origin: {}\nGit branch: {}\nCommit hash: {}".format(
    gitOrigin, gitBranch, gitHash)
      
  baseState = 'FAILED'
  messageText = "Pulling updates for Cassiopee, Fast and all "\
    "PModules:\n{}\n\n{}\n\n\n".format(52*'-', gitInfo)
  messageText += '{:^20} | {:^15} | {:^30} | {:^10}\n{}\n'.format(
      "PROD.", "PCKG.", "DATE", "STATUS", 85*'-')
  for log_machine in log_entries:
    prod = log_machine[0]
    pckg = log_machine[1]
    date = strptime(log_machine[2], "%y%m%d-%H%M%S")
    date = strftime("%d/%m/%y at %T", date)
    status = log_machine[3]
    messageText += '{:^20} | {:^15} | {:^30} | {:^10}\n'.format(
      prod, pckg, date, status)
    
  messageText += '\n\nIf the prod. you wish to use is marked as FAILED, '\
    'please contact the maintainers:\nchristophe.benoit@onera.fr, '\
    'vincent.casseau@onera.fr'
  
  notify(recipients=recipients,
         messageSubject="[Checkout Cassiopee] State: {}".format(baseState),
         messageText=messageText)
