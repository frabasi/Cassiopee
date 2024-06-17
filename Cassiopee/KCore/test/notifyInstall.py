# Send a notification by email to a list of recipients about the installation
# status of different prods. Please set the environment variable CASSIOPEE_EMAIL
# Usage: python notifyInstall.py --recipients='a.b@onera.fr c.d@onera.fr'
import os
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
  script_args = parseArgs()
  recipients = script_args.recipients.split(' ')
  if not recipients[0]: recipients = []
  
  # Check install status
  log_entries = []
  with open('/stck/cassiope/git/logs/installation_status.txt', 'r') as f:
    for line in f:
      log_entries.append(line.strip().split(' - '))
  log_entries.sort(key=lambda x: x[1], reverse=True)
  
  baseState = 'OK'
  messageText = "Installation of Cassiopee, Fast and all PModules:\n{}\n\n".format(48*'-')
  for log_machine in log_entries:
    prod = log_machine[0]
    date = strptime(log_machine[1], "%y%m%d-%H%M%S")
    date = strftime("%d/%m/%y at %T", date)
    status = log_machine[2]
    messageText += '{:>20} |      {}      | {:>10}\n'.format(prod, date, status)
    if 'FAILED' in log_machine: baseState = 'FAILED'
  
  try:
    from KCore.notify import notify
    notify(recipients=recipients,
           messageSubject="[Installation] State: {}".format(baseState),
           messageText=messageText)
  except ImportError:
    print("Error: KCore is required to import notify.")
    sys.exit()
